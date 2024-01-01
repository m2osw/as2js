// Copyright (c) 2005-2024  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/as2js
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// self
//
#include    "as2js/compiler.h"

#include    "as2js/exception.h"
#include    "as2js/message.h"
#include    "as2js/parser.h"

// private classes
//
#include    "as2js/file/database.h"
#include    "as2js/file/resources.h"



// snapdev
//
#include    <snapdev/enumerate.h>
#include    <snapdev/glob_to_list.h>
#include    <snapdev/join_strings.h>
#include    <snapdev/tokenize_string.h>


// C++
//
#include    <algorithm>
#include    <cstring>


// C
//
#include    <dirent.h>


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{


namespace
{


// The following globals are read only once and you can compile
// many times without having to reload them.
//
// the resource file information
//
resources                   g_rc;

// the global imports (those which are automatic and
// define the intrinsic functions and types of the language)
//
node::pointer_t             g_global_import;

// the system imports (this is specific to the system you
// are using this compiler for; it defines the system)
//
node::pointer_t             g_system_import;

// the native imports (this is specific to your system
// environment, it defines objects in your environment)
//
node::pointer_t             g_native_import;

// the database handling all the packages and their name
// so we can quickly find which package to import when
// a given name is used
//
database::pointer_t         g_db;

// whether the database was loaded (true) or not (false)
//
bool                        g_db_loaded = false;

// Search for a named element:
// <package name>{.<package name>}.<class, function, variable name>
// TODO: add support for '*' in <package name>
database::element::pointer_t find_element(
      std::string const & package_name
    , std::string const & element_name
    , char const * type)
{
    database::package::vector_t packages(g_db->find_packages(package_name));
    for(auto pt(packages.begin()); pt != packages.end(); ++pt)
    {
        database::element::vector_t elements((*pt)->find_elements(element_name));
        for(auto et(elements.begin()); et != elements.end(); ++et)
        {
            if(!type
            || (*et)->get_type() == type)
            {
                return *et;
            }
        }
    }

    return database::element::pointer_t();
}


void add_element(
      std::string const & package_name
    , std::string const & element_name
    , node::pointer_t element
    , char const * type)
{
    database::package::pointer_t p(g_db->add_package(package_name));
    database::element::pointer_t e(p->add_element(element_name));
    e->set_type(type);
    e->set_filename(element->get_position().get_filename());
    e->set_line(element->get_position().get_line());
}


}
// no name namespace







/** \brief Get the filename of a package.
 *
 */
std::string compiler::get_package_filename(char const * package_info)
{
    for(int cnt(0); *package_info != '\0';)
    {
        ++package_info;
        if(package_info[-1] == ' ')
        {
            ++cnt;
            if(cnt >= 3)
            {
                break;
            }
        }
    }
    if(*package_info != '"')
    {
        return std::string();
    }
    ++package_info;
    char const *name = package_info;
    while(*package_info != '"' && *package_info != '\0')
    {
        ++package_info;
    }

    return std::string(name, package_info - name);
}


/** \brief Find a module, load it if necessary.
 *
 * If the module was already loaded, return a pointer to the existing
 * tree of nodes.
 *
 * If the module was not yet loaded, try to load it. If the file cannot
 * be found or the file cannot be compiled, a fatal error is emitted
 * and the process stops.
 *
 * \note
 * At this point this function either returns true because it found
 * the named module, or it throws exception_exit. So the
 * \p result parameter is always set on a normal return.
 *
 * \param[in] filename  The name of the module to be loaded
 * \param[in,out] result  The shared pointer to the resulting root node.
 *
 * \return true if the module was found.
 */
bool compiler::find_module(std::string const & filename, node::pointer_t & result)
{
    // module already loaded?
    module_map_t::const_iterator existing_module(f_modules.find(filename));
    if(existing_module != f_modules.end())
    {
        result = existing_module->second;
        return true;
    }

    // we could not find this module in our cache, try to load it
    //
    base_stream::pointer_t in;
    if(f_input_retriever)
    {
        in = f_input_retriever->retrieve(filename);
    }
    if(in == nullptr)
    {
        input_stream<std::ifstream>::pointer_t input(std::make_shared<input_stream<std::ifstream>>());
        input->get_position().set_filename(filename);
        input->open(filename);
        if(!input->is_open())
        {
            return false;
        }
        in = input;
    }

    // Parse the file in result
    //
    parser::pointer_t p(std::make_shared<parser>(in, f_options));
    result = p->parse();
    p.reset();

#if 0
//std::cerr << "+++++\n \"" << filename << "\" module:\n" << *result << "\n+++++\n";
std::cerr << "+++++++++++++++++++++++++++ Loading \"" << filename << "\" +++++++++++++++++++++++++++++++\n";
#endif

    if(!result)
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_CANNOT_COMPILE, in->get_position());
        msg << "could not compile module file \"" << filename << "\".";
        throw as2js_exit("could not compile module file.", 1);
    }

    // save the newly loaded module
    f_modules[filename] = result;

    return true;
}



/** \brief Load a module as specified by \p module and \p file.
 *
 * This function loads the specified module. The filename is defined
 * as the path found in the .rc file, followed by the module name,
 * followed by the file name:
 *
 * \code
 * <rc.path>/<module>/<file>
 * \endcode
 *
 * The function always returns a pointer. If the module cannot be
 * loaded, an error is generated and the compiler exists with a
 * fatal error.
 *
 * \param[in] module  The name of the module to be loaded.
 * \param[in] file  The name of the file to load from that module.
 *
 * \return The pointer to the module loaded.
 */
node::pointer_t compiler::load_module(
      std::string const & module
    , std::string const & file)
{
    for(auto path : g_rc.get_scripts())
    {
        path += '/';
        path += module;
        path += '/';
        path += file;

        node::pointer_t result;
        if(find_module(path, result))
        {
            return result;
        }
    }

    position pos;
    pos.set_filename(file);
    message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND, pos);
    msg << "module file \""
        << file
        << "\" not found in any of the paths \""
        << snapdev::join_strings(g_rc.get_scripts(), ":")
        << "\".";
    throw as2js_exit(msg.str(), 1);
}




void compiler::find_packages_add_database_entry(
      std::string const & package_name
    , node::pointer_t & element
    , char const * type)
{
    // here, we totally ignore internal, private
    // and false entries right away
    //
    if(get_attribute(element, attribute_t::NODE_ATTR_PRIVATE)
    || get_attribute(element, attribute_t::NODE_ATTR_FALSE)
    || get_attribute(element, attribute_t::NODE_ATTR_INTERNAL))
    {
        return;
    }

    add_element(package_name, element->get_string(), element, type);
}



/** \brief Find elements that a package declares.
 *
 * A JavaScript package can define the following type of declarations:
 *
 * * functions
 * * variables
 * * classes
 * * enumerations
 *
 * These elements are saved in the database so that way they can very
 * quickly be found later when import and a reference are used.
 *
 * \note
 * The function takes the name of the package as a parameter so we do not
 * have to search it (i.e. we'd have to search the parent list and find
 * the NODE_PACKAGE and get the string on that node). Also this works with
 * definitions of sub-packages.
 *
 * \param[in] package  A pointer to the NODE_DIRECTIVE_LIST of the package.
 * \param[in] package_name  The name of the package.
 */
void compiler::find_packages_save_package_elements(
      node::pointer_t package
    , std::string const & package_name)
{
    std::size_t const max_children(package->get_children_size());
    for(std::size_t idx = 0; idx < max_children; ++idx)
    {
        node::pointer_t child(package->get_child(idx));
        switch(child->get_type())
        {
        case node_t::NODE_DIRECTIVE_LIST:
            find_packages_save_package_elements(child, package_name); // recursive
            break;

        case node_t::NODE_CLASS:
            find_packages_add_database_entry(
                    package_name,
                    child,
                    "class"
                );
            break;

        case node_t::NODE_FUNCTION:
            {
                // we do not save prototypes, that is tested later
                //
                char const * type;
                if(child->get_flag(flag_t::NODE_FUNCTION_FLAG_GETTER))
                {
                    type = "getter";
                }
                else if(child->get_flag(flag_t::NODE_FUNCTION_FLAG_SETTER))
                {
                    type = "setter";
                }
                else
                {
                    type = "function";
                }
                find_packages_add_database_entry(
                        package_name,
                        child,
                        type
                    );
            }
            break;

        case node_t::NODE_VAR:
            {
                std::size_t const vcnt(child->get_children_size());
                for(std::size_t v(0); v < vcnt; ++v)
                {
                    node::pointer_t variable_node(child->get_child(v));
                    // we do not save the variable type,
                    // it would not help resolution
                    //
                    find_packages_add_database_entry(
                            package_name,
                            variable_node,
                            "variable"
                        );
                }
            }
            break;

        case node_t::NODE_ENUM:
            find_packages_add_database_entry(
                    package_name,
                    child,
                    "enumeration"
                );
            break;

        case node_t::NODE_PACKAGE:
            {
                // sub-package
                //
                node::pointer_t list(child->get_child(0));
                std::string name(package_name);
                name += '.';
                name += child->get_string();
                find_packages_save_package_elements(list, name); // recursive
            }
            break;

        default:
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_SUPPORTED, child->get_position());
            msg << "packages do not yet support \""
                << child->get_type_name()
                << "\" declarations.\n";
            break;

        }
    }
}


// this function searches the tree for packages (it stops at classes,
// functions, and other such blocks)
void compiler::find_packages_directive_list(node::pointer_t list)
{
    std::size_t const max_children(list->get_children_size());
    for(std::size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t child(list->get_child(idx));
        if(child->get_type() == node_t::NODE_DIRECTIVE_LIST)
        {
            find_packages_directive_list(child);
        }
        else if(child->get_type() == node_t::NODE_PACKAGE)
        {
            // found a package
            //
            node::pointer_t directive_list_node(child->get_child(0));
            find_packages_save_package_elements(directive_list_node, child->get_string());
        }
    }
}


void compiler::find_packages(node::pointer_t program_node)
{
    if(program_node->get_type() != node_t::NODE_PROGRAM)
    {
        return;
    }

    find_packages_directive_list(program_node);
}


void compiler::load_internal_packages(std::string const & module)
{
    for(auto path : g_rc.get_scripts())
    {
        path += '/';
        path += module;
        path += "/*.ajs";

        snapdev::glob_to_list<std::list<std::string>> ajs_files;
        if(ajs_files.read_path<
                snapdev::glob_to_list_flag_t::GLOB_FLAG_TILDE,
                snapdev::glob_to_list_flag_t::GLOB_FLAG_RECURSIVE>(path))
        {
            if(!ajs_files.empty())
            {
                snapdev::enumerate(
                      ajs_files
                    , [this, module](int index, std::string const & filename)
                    {
                        snapdev::NOT_USED(index);

                        std::string const basename(snapdev::pathinfo::basename(filename));
                        if(basename == "as2js_init.ajs")
                        {
                            return;
                        }

                        // we got a file of interest
                        //
                        // TODO: we want to keep this package in RAM since
                        //       we already parsed it!
                        //
                        node::pointer_t p(load_module(module, basename));

                        // now we can search the package in the actual code
                        //
                        find_packages(p);
                    });

                return;
            }
        }
    }

    message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INSTALLATION);
    msg << "cannot find module \""
        << module
        << "\" in any of the script folders: \""
        << snapdev::join_strings(g_rc.get_scripts(), ":")
        << "\".\n";
    throw as2js_exit(msg.str(), 1);
}


void compiler::import(node::pointer_t & import_node)
{
    // if we have the IMPLEMENTS flag set, then we must make sure
    // that the corresponding package is compiled
    //
    if(!import_node->get_flag(flag_t::NODE_IMPORT_FLAG_IMPLEMENTS))
    {
        return;
    }

    // find the package
    //
    node::pointer_t package;

    // search in this program
    //
    package = find_package(f_program, import_node->get_string());
    if(package == nullptr)
    {
        // not in this program, search externals
        //
        node::pointer_t program_node;
        std::string any_name("*");
        if(find_external_package(import_node, any_name, program_node))
        {
            // got externals, search those now
            //
            package = find_package(program_node, import_node->get_string());
        }
        if(package == nullptr)
        {
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_FOUND, import_node->get_position());
            msg << "cannot find package \"" << import_node->get_string() << "\".";
            return;
        }
    }

    // make sure it is compiled (once)
    //
    bool const was_referenced(package->get_flag(flag_t::NODE_PACKAGE_FLAG_REFERENCED));
    package->set_flag(flag_t::NODE_PACKAGE_FLAG_REFERENCED, true);
    if(!was_referenced)
    {
        directive_list(package);
    }
}






node::pointer_t compiler::find_package(
      node::pointer_t list
    , std::string const & name)
{
    node_lock ln(list);
    std::size_t const max_children(list->get_children_size());
    for(std::size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t child(list->get_child(idx));
        if(child->get_type() == node_t::NODE_DIRECTIVE_LIST)
        {
            node::pointer_t package(find_package(child, name));  // recursive
            if(package)
            {
                return package;
            }
        }
        else if(child->get_type() == node_t::NODE_PACKAGE)
        {
            if(child->get_string() == name)
            {
                // found it!
                return child;
            }
        }
    }

    // not found
    return node::pointer_t();
}


bool compiler::find_external_package(
      node::pointer_t import_node
    , std::string const & name
    , node::pointer_t & program_node)
{
    // search a package which has an element named 'name'
    // and has a name which match the identifier specified in 'import'
    //
    database::element::pointer_t e(find_element(import_node->get_string(), name, nullptr));
    if(!e)
    {
        // not found!
        //
        return false;
    }

    std::string const filename(e->get_filename());

    // found it, lets get a node for it
    //
    if(!find_module(filename, program_node))
    {
        return false;
        //message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND, input->get_position());
        //msg << "could not find module file \"" << filename << "\" (did it move?).";
        //throw as2js_exit(msg.str(), 1);
    }

    // at this time this will not happen because if the find_module()
    // function fails, it throws exception_exit(..., 1);
    //
    if(program_node == nullptr)
    {
        return false;
    }

    return true;
}


bool compiler::check_import(
      node::pointer_t & import_node
    , node::pointer_t & resolution
    , std::string const & name
    , node::pointer_t params
    , int search_flags)
{
    // search for a package within this program
    // (I am not too sure, but according to the spec. you can very well
    // have a package within any script file)
    if(find_package_item(f_program, import_node, resolution, name, params, search_flags))
    {
        return true;
    }

    node::pointer_t program_node;
    if(!find_external_package(import_node, name, program_node))
    {
        return false;
    }

    return find_package_item(program_node, import_node, resolution, name, params, search_flags | SEARCH_FLAG_PACKAGE_MUST_EXIST);
}


bool compiler::find_package_item(
      node::pointer_t program_node
    , node::pointer_t import_node
    , node::pointer_t & resolution
    , std::string const & name
    , node::pointer_t params
    , int const search_flags)
{
    node::pointer_t package_node(find_package(program_node, import_node->get_string()));

    if(!package_node)
    {
        if((search_flags & SEARCH_FLAG_PACKAGE_MUST_EXIST) != 0)
        {
            // this is a bad error! we should always find the
            // packages in this case (i.e. when looking using the
            // database.)
            message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INTERNAL_ERROR, import_node->get_position());
            msg << "cannot find package \"" << import_node->get_string() << "\" in any of the previously registered packages.";
            throw as2js_exit(msg.str(), 1);
        }
        return false;
    }

    if(package_node->get_children_size() == 0)
    {
        return false;
    }

    // setup labels (only the first time around)
    if(!package_node->get_flag(flag_t::NODE_PACKAGE_FLAG_FOUND_LABELS))
    {
        package_node->set_flag(flag_t::NODE_PACKAGE_FLAG_FOUND_LABELS, true);
        node::pointer_t child(package_node->get_child(0));
        find_labels(package_node, child);
    }

    // search the name of the class/function/variable we're
    // searching for in this package:

    // TODO: Hmmm... could we have the actual node instead?
    node::pointer_t id(package_node->create_replacement(node_t::NODE_IDENTIFIER));
    id->set_string(name);

    if(!find_field(package_node, id, resolution, params, node::pointer_t(), search_flags))
    {
        return false;
    }

    // TODO: Can we have an empty resolution here?!
    if(resolution)
    {
        if(get_attribute(resolution, attribute_t::NODE_ATTR_PRIVATE))
        {
            // it is private, we cannot use this item
            // from outside whether it is in the
            // package or a sub-class
            return false;
        }

        if(get_attribute(resolution, attribute_t::NODE_ATTR_INTERNAL))
        {
            // it is internal we can only use it from
            // another package
            node::pointer_t parent(import_node);
            for(;;)
            {
                parent = parent->get_parent();
                if(!parent)
                {
                    return false;
                }
                if(parent->get_type() == node_t::NODE_PACKAGE)
                {
                    // found the package mark
                    break;
                }
                if(parent->get_type() == node_t::NODE_ROOT
                || parent->get_type() == node_t::NODE_PROGRAM)
                {
                    return false;
                }
            }
        }
    }

    // make sure it is compiled (once)
    bool const was_referenced(package_node->get_flag(flag_t::NODE_PACKAGE_FLAG_REFERENCED));
    package_node->set_flag(flag_t::NODE_PACKAGE_FLAG_REFERENCED, true);
    if(!was_referenced)
    {
        directive_list(package_node);
    }

    return true;
}


void compiler::internal_imports()
{
    if(g_native_import == nullptr)
    {
        // read the resource file
        //
        g_rc.init(static_cast<bool>(f_input_retriever));

        // TBD: at this point we only have native scripts
        //
        //      at some point, we want to have browser scripts in order to
        //      verify code that runs on browsers; however that should
        //      probably be an extension (not auto-imported)
        //
        //      and one day maybe definitions of extensions such as jQuery
        //
        g_native_import = load_module("native", "as2js_init.ajs");
    }

    if(g_db == nullptr)
    {
        g_db = std::make_shared<database>();
    }
    if(!g_db->load(g_rc.get_db()))
    {
        message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_UNEXPECTED_DATABASE);
        msg << "Failed reading the compiler database. You may need to delete it and try again or fix the resource file to point to the right file.";
        return;
    }

    if(!g_db_loaded)
    {
        g_db_loaded = true;

        // the ECMAScript low level definitions
        //
        load_internal_packages("native");

        // this saves the internal packages info for fast query
        // on next invocations
        //
        g_db->save();
    }
}


void compiler::clean()
{
    g_db_loaded = false;
    g_db.reset();
    g_native_import.reset();
}


bool compiler::resolve_name(
      node::pointer_t list
    , node::pointer_t id
    , node::pointer_t & resolution
    , node::pointer_t params
    , node::pointer_t all_matches
    , int const search_flags)
{
//std::cerr << " +++ resolve_name()\n";
    restore_flags save_flags(this);

    // just in case the caller is reusing the same node
    resolution.reset();

    // resolution may includes a member (a.b) and the resolution is the
    // last field name
    //
    node_t id_type(id->get_type());
    if(id_type == node_t::NODE_MEMBER)
    {
        // child 0 is the variable name, child 1 is the field name
        //
        if(id->get_children_size() != 2)
        {
            throw internal_error(std::string("compiler_package:compiler::resolve_name() called with a MEMBER which does not have exactly two children."));
        }
        node::pointer_t name(id->get_child(0));
        if(!resolve_name(list, name, resolution, params, all_matches, search_flags))  // recursive
        {
            // we could not find 'name' so we are hosed anyway
            // the callee should already have generated an error
            //
            return false;
        }
        list = resolution;
        resolution.reset();
//std::cerr << "-- MEMBER is:\n" << *id << "\n";
        id = id->get_child(1);
        id_type = id->get_type();
    }

    // in some cases we may want to resolve a name specified in a string
    // (i.e. test["me"])
    //
    if(id_type != node_t::NODE_IDENTIFIER
    && id_type != node_t::NODE_VIDENTIFIER
    && id_type != node_t::NODE_STRING)
    {
//std::cerr << "-- the list:\n" << *list << " ++> the id_type is:\n" << *id << "\n";
//abort();
        throw internal_error(
              std::string("compiler_package:compiler::resolve_name() was called with a \"NODE_")
            + id->get_type_name()
            + "\", which is not a NODE_[V]IDENTIFIER or NODE_STRING.");
    }

    // already typed?
    {
        node::pointer_t type(id->get_type_node());
        if(type != nullptr)
        {
            // TBD: this is probably wrong if the type is a function?
            //
            resolution = type;
            return true;
        }
    }

    //
    // Search for the parent list of directives; in that list, search
    // for the identifier; if not found, try again with the parent
    // of that list of directives (unless we find an import in which
    // case we first try the import)
    //
    // Note that the currently effective with()'s and use namespace's
    // are defined in the f_scope variable. This is used here to know
    // whether the name matches an entry or not.
    //

    // if we start by finding a function then we create a list in the
    // all_matches (references really); if we find something else first
    // then we return that immediately

    node::pointer_t parent(list->get_parent());
    if(parent->get_type() == node_t::NODE_WITH)
    {
        // we are currently defining the WITH object, skip the
        // WITH itself!
        //
        list = parent;
    }
    int module(0);        // 0 is user module being compiled
    for(;;)
    {
        // we will start searching at this offset; first backward
        // and then forward
        //
        std::size_t offset(0);

        // This function should never be called from program()
        // also, 'id' cannot be a directive list (it has to be an
        // identifier, a member or a string!)
        //
        // For these reasons, we can start the following loop with
        // a get_parent() in all cases.
        //
        if(module == 0)
        {
            // when we were inside the function parameter
            // list we do not want to check out the function
            // otherwise we could have a forward search of
            // the parameters which we disallow (only backward
            // search is allowed in that list)
            //
            if(list->get_type() == node_t::NODE_PARAMETERS)
            {
                list = list->get_parent();
                if(list == nullptr)
                {
                    throw internal_error("compiler_package:compiler::resolve_name() got a nullptr parent without finding NODE_ROOT first (NODE_PARAMETERS).");
                }
            }

            for(bool more(true); more; )
            {
                offset = list->get_offset();
                list = list->get_parent();
                if(list == nullptr)
                {
                    throw internal_error("compiler_package:compiler::resolve_name() got a nullptr parent without finding NODE_ROOT first.");
                }
                switch(list->get_type())
                {
                case node_t::NODE_ROOT:
                    throw internal_error("compiler_package:compiler::resolve_name() found the NODE_ROOT while searching for a parent.");

                case node_t::NODE_EXTENDS:
                case node_t::NODE_IMPLEMENTS:
                    list = list->get_parent();
                    if(list == nullptr)
                    {
                        throw internal_error("compiler_package:compiler::resolve_name() got a nullptr parent without finding NODE_ROOT first (NODE_EXTENDS/NODE_IMPLEMENTS).");
                    }
                    break;

                case node_t::NODE_DIRECTIVE_LIST:
                case node_t::NODE_FOR:
                case node_t::NODE_WITH:
                //case node_t::NODE_PACKAGE: -- not necessary, the first item is a NODE_DIRECTIVE_LIST
                case node_t::NODE_PROGRAM:
                case node_t::NODE_FUNCTION:
                case node_t::NODE_PARAMETERS:
                case node_t::NODE_ENUM:
                case node_t::NODE_CATCH:
                case node_t::NODE_CLASS:
                case node_t::NODE_INTERFACE:
                    more = false;
                    break;

                default:
                    break;

                }
            }
        }

        if(list->get_type() == node_t::NODE_PROGRAM
        || module != 0)
        {
            // not resolved
            //
            switch(module)
            {
            case 0:
                module = 1;
                if(g_global_import != nullptr
                && g_global_import->get_children_size() > 0)
                {
                    list = g_global_import->get_child(0);
                    break;
                }
#if __cplusplus >= 201700
                [[fallthrough]];
#endif
            case 1:
                module = 2;
                if(g_system_import != nullptr
                && g_system_import->get_children_size() > 0)
                {
                    list = g_system_import->get_child(0);
                    break;
                }
#if __cplusplus >= 201700
                [[fallthrough]];
#endif
            case 2:
                module = 3;
                if(g_native_import != nullptr
                && g_native_import->get_children_size() > 0)
                {
                    list = g_native_import->get_child(0);
                    break;
                }
#if __cplusplus >= 201700
                [[fallthrough]];
#endif
            case 3:
                // no more default list of directives...
                module = 4;
                break;

            }
            offset = 0;
        }
        if(module == 4)
        {
            // did not find a variable and such, but
            // we may have found a function (see below
            // after the forever loop breaking here)
            //
            break;
        }

        node_lock ln(list);
        std::size_t const max_children(list->get_children_size());
        switch(list->get_type())
        {
        case node_t::NODE_DIRECTIVE_LIST:
            {
                // okay! we have got a list of directives
                // backward loop up first since in 99% of cases that
                // will be enough...
                //
                if(offset >= max_children)
                {
                    throw internal_error("compiler::resolve_name(): somehow offset >= max_children is out of range");
                }
                std::size_t idx(offset);
                while(idx > 0)
                {
                    --idx;
                    if(check_name(list, idx, resolution, id, params, all_matches, search_flags))
                    {
                        if(funcs_name(resolution, all_matches))
                        {
                            return true;
                        }
                    }
                }

                // forward look up is also available in ECMAScript...
                // (actually necessary in case function A calls function B
                // and function B calls function A).
                //
                for(idx = offset; idx < max_children; ++idx)
                {
                    if(check_name(list, idx, resolution, id, params, all_matches, search_flags))
                    {
                        // TODO: if it is a variable it needs
                        //       to be a constant...
                        //
                        if(funcs_name(resolution, all_matches))
                        {
                            return true;
                        }
                    }
                }
            }
            break;

        case node_t::NODE_FOR:
            // the first member of a for can include variable definitions
            //
            if(max_children > 0
            && check_name(list, 0, resolution, id, params, all_matches, search_flags))
            {
                if(funcs_name(resolution, all_matches))
                {
                    return true;
                }
            }
            break;

#if 0
        case node_t::NODE_PACKAGE:
            // From inside a package, we have an implicit
            //    IMPORT <package name>;
            //
            // This is required to enable a multiple files
            // package definition which ease the development
            // of really large packages.
            if(check_import(list, resolution, id->get_string(), params, search_flags))
            {
                return true;
            }
            break;
#endif

        case node_t::NODE_WITH:
            {
                if(max_children != 2)
                {
                    break;
                }
                // ha! we found a valid WITH instruction, let's
                // search for this name in the corresponding
                // object type instead (i.e. a field of the object)
                node::pointer_t type(list->get_child(0));
                if(type != nullptr)
                {
                    node::pointer_t link(type->get_instance());
                    if(link != nullptr)
                    {
                        //node::pointer_t all_matches(link->create_replacement(node_t::NODE_LIST));
                        if(resolve_field(link, id, resolution, params, all_matches, search_flags))
                        {
                            // Mark this identifier as a
                            // reference to a WITH object
                            id->set_flag(flag_t::NODE_IDENTIFIER_FLAG_WITH, true);

                            // TODO: we certainly want to compare
                            //       all the field functions and the
                            //       other functions... at this time,
                            //       err if we get a field function
                            //       and others are ignored!
                            //
                            if(all_matches->get_children_size() != 0)
                            {
                                throw internal_error("at this time we do not support functions here (under a with)");
                            }
                            return true;
                        }
                    }
                }
            }
            break;

        case node_t::NODE_FUNCTION:
            // if identifier is marked as a type, then skip testing
            // the function parameters since those cannot be type
            // declarations
            //
            if(!get_attribute(id, attribute_t::NODE_ATTR_TYPE))
            {
                // search the list of parameters for a corresponding name
                for(size_t idx(0); idx < max_children; ++idx)
                {
                    node::pointer_t parameters_node(list->get_child(idx));
                    if(parameters_node->get_type() == node_t::NODE_PARAMETERS)
                    {
                        node_lock parameters_ln(parameters_node);
                        size_t const cnt(parameters_node->get_children_size());
                        for(size_t j(0); j < cnt; ++j)
                        {
                            if(check_name(parameters_node, j, resolution, id, params, all_matches, search_flags))
                            {
                                if(funcs_name(resolution, all_matches))
                                {
                                    return true;
                                }
                            }
                        }
                        break;
                    }
                }
            }
            break;

        case node_t::NODE_PARAMETERS:
            {
                // Wow! I cannot believe I am implementing this...
                // So we will be able to reference the previous
                // parameters in the default value of the following
                // parameters; and that makes sense, it is available
                // in C++ templates, right?!
                // And guess what, that is just this little loop.
                // That is it. Big deal, hey?! 8-)
                if(offset >= max_children)
                {
                    throw internal_error("somehow an offset is out of range");
                }
                size_t idx(offset);
                while(idx > 0)
                {
                    idx--;
                    if(check_name(list, idx, resolution, id, params, all_matches, search_flags))
                    {
                        if(funcs_name(resolution, all_matches))
                        {
                            return true;
                        }
                    }
                }
            }
            break;

        case node_t::NODE_CATCH:
            {
                // a catch can have a parameter of its own
                node::pointer_t parameters_node(list->get_child(0));
                if(parameters_node->get_children_size() > 0)
                {
                    if(check_name(parameters_node, 0, resolution, id, params, all_matches, search_flags))
                    {
                        if(funcs_name(resolution, all_matches))
                        {
                            return true;
                        }
                    }
                }
            }
            break;

        case node_t::NODE_ENUM:
            // first we check whether the name of the enum is what
            // is being referenced (i.e. the type)
            if(id->get_string() == list->get_string())
            {
                resolution = list;
                resolution->set_flag(flag_t::NODE_ENUM_FLAG_INUSE, true);
                return true;
            }

            // inside an enum we have references to other
            // identifiers of that enum and these need to be
            // checked here
            //
            // And note that these are not in any way affected
            // by scope attributes
            for(size_t idx(0); idx < max_children; ++idx)
            {
                node::pointer_t entry(list->get_child(idx));
                if(entry->get_type() == node_t::NODE_VARIABLE)
                {
                    if(id->get_string() == entry->get_string())
                    {
                        // this cannot be a function, right? so the following
                        // call is probably not really useful
                        resolution = entry;
                        if(funcs_name(resolution, all_matches))
                        {
                            resolution->set_flag(flag_t::NODE_VARIABLE_FLAG_INUSE, true);
                            return true;
                        }
                    }
                }
                // else -- probably a NODE_TYPE
            }
            break;

        case node_t::NODE_CLASS:
        case node_t::NODE_INTERFACE:
            // // if the ID is a type and the name is the same as the
            // // class name, then we found what we were looking for
            // //
            // if(get_attribute(id, attribute_t::NODE_ATTR_TYPE)
            // && id->get_string() == list->get_string())
            // {
            //     resolution = list;
            //     return true;
            // }

            // We want to search the extends and implements declarations as well
            //
            if(find_in_extends(list, id, resolution, params, all_matches, search_flags))
            {
                if(funcs_name(resolution, all_matches))
                {
                    return true;
                }
            }
            break;

        default:
            // this could happen if our tree was to change and we do not
            // properly update this function
            throw internal_error("compiler::resolve_name(): compiler_package: unhandled type.");

        }
    }

    resolution.reset();

//std::cerr << "--- final selection for function(s):\n" << *all_matches << "\n";
    if(all_matches != nullptr
    && all_matches->get_children_size() != 0)
    {
        if(select_best_func(all_matches, resolution))
        {
            return true;
        }
    }

    print_search_errors(id);

    return false;
}



} // namespace as2js
// vim: ts=4 sw=4 et
