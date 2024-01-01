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
#pragma once

// self
//
#include    <as2js/optimizer.h>
#include    <as2js/stream.h>
#include    <as2js/options.h>


namespace as2js
{



/** \brief Compile an Advanced JavaScript tree of node.
 *
 * Once a program was parsed, you need to compile it. This
 * mainly means resolving the references (i.e. identifiers),
 * which may generate the loading of libraries specified in
 * import instructions (note that some import instructions
 * are automatic for the global and native environments).
 *
 * The code to compile, assuming you already ran the parser,
 * looks like this:
 *
 * \code
 *    // use the same options as for the parser
 *    compiler::pointer_t compiler(std::make_shared<as2js::compiler>(options));
 *    error_count = compiler->compile(root);
 * \endcode
 *
 * The compile() function returns the number of errors
 * encountered while compiling. The root parameter is
 * what was returned by the parser::parse() function.
 */
class compiler
{
public:
    typedef std::shared_ptr<compiler>   pointer_t;

                                compiler(options::pointer_t options);
    virtual                     ~compiler();

    input_retriever::pointer_t  set_input_retriever(input_retriever::pointer_t retriever);
    int                         compile(node::pointer_t & root);
    void                        resolve_internal_type(node::pointer_t parent, char const * type, node::pointer_t & /*out*/ resolution);
    static void                 clean();

private:
    typedef uint32_t            search_error_t;
    typedef uint32_t            search_flag_t;

    static search_error_t const SEARCH_ERROR_NONE                   = 0x00000000;
    static search_error_t const SEARCH_ERROR_PRIVATE                = 0x00000001;
    static search_error_t const SEARCH_ERROR_PROTECTED              = 0x00000002;
    static search_error_t const SEARCH_ERROR_PROTOTYPE              = 0x00000004;
    static search_error_t const SEARCH_ERROR_WRONG_PRIVATE          = 0x00000008;
    static search_error_t const SEARCH_ERROR_WRONG_PROTECTED        = 0x00000010;
    static search_error_t const SEARCH_ERROR_PRIVATE_PACKAGE        = 0x00000020;
    static search_error_t const SEARCH_ERROR_EXPECTED_STATIC_MEMBER = 0x00000040;

    static search_flag_t const  SEARCH_FLAG_NO_PARSING              = 0x00000001;    // avoid parsing variables
    static search_flag_t const  SEARCH_FLAG_GETTER                  = 0x00000002;    // accept getters (reading)
    static search_flag_t const  SEARCH_FLAG_SETTER                  = 0x00000004;    // accept setters (writing)
    static search_flag_t const  SEARCH_FLAG_PACKAGE_MUST_EXIST      = 0x00000008;    // weather the package has to exist
    static search_flag_t const  SEARCH_FLAG_RESOLVING_CALL          = 0x00000010;    // resolving a NODE_CALL

    typedef std::map<std::string, node::pointer_t>   module_map_t;

    // automate the restoration of the error flags
    class restore_flags
    {
    public:
        restore_flags(compiler * compiler)
            : f_compiler(compiler)
            , f_org_flags(f_compiler->get_err_flags())
        {
            f_compiler->set_err_flags(0);
        }

        restore_flags(restore_flags const & rhs) = delete;

        ~restore_flags()
        {
            f_compiler->set_err_flags(f_org_flags);
        }

        restore_flags & operator = (restore_flags const & rhs) = delete;

    private:
        compiler *      f_compiler = nullptr;
        int             f_org_flags = 0;
    };


    // functions used to load the internal imports
    void                internal_imports();
    node::pointer_t     load_module(std::string const & module, std::string const & file);
    void                load_internal_packages(std::string const & module);
    void                read_db();
    void                write_db();
    bool                find_module(std::string const & filename, node::pointer_t & result);
    void                find_packages_add_database_entry(std::string const & package_name, node::pointer_t & element, char const * type);
    void                find_packages_save_package_elements(node::pointer_t package, std::string const & package_name);
    void                find_packages_directive_list(node::pointer_t list);
    void                find_packages(node::pointer_t program);
    std::string         get_package_filename(char const * package_info);

    void                add_variable(node::pointer_t variable);
    bool                are_objects_derived_from_one_another(node::pointer_t derived_class, node::pointer_t super_class, node::pointer_t & /*out*/ the_super_class);
    void                array_operator(node::pointer_t & expr);
    void                assignment_operator(node::pointer_t expr);
    bool                best_param_match(node::pointer_t & /*in,out*/ best, node::pointer_t match);
    bool                best_param_match_derived_from(node::pointer_t & /*in,out*/ best, node::pointer_t match);
    void                binary_operator(node::pointer_t & expr);
    void                break_continue(node::pointer_t & break_node);
    void                call_add_missing_params(node::pointer_t call, node::pointer_t params);
    void                call_operator(node::pointer_t & expr);
    void                can_instantiate_type(node::pointer_t expr);
    void                case_directive(node::pointer_t & case_node);
    void                catch_directive(node::pointer_t & catch_node);
    bool                check_field(node::pointer_t link, node::pointer_t field, node::pointer_t & /*out*/ resolution, node::pointer_t params, node::pointer_t all_matches, int const search_flags);
    bool                check_final_functions(node::pointer_t & function_node, node::pointer_t & class_node);
    bool                check_function(node::pointer_t function_node, node::pointer_t & /*out*/ resolution, std::string const & name, node::pointer_t params, int const search_flags);
    int                 check_function_with_params(node::pointer_t function_node, node::pointer_t params, node::pointer_t /*in,out*/ all_matches);
    bool                check_import(node::pointer_t & child, node::pointer_t & /*out*/ resolution, std::string const & name, node::pointer_t params, int const search_flags);
    void                check_member(node::pointer_t ref, node::pointer_t field, node::pointer_t field_name);
    bool                check_name(node::pointer_t list, int idx, node::pointer_t & /*out*/ resolution, node::pointer_t id, node::pointer_t params, node::pointer_t all_matches, int const search_flags);
    void                check_super_validity(node::pointer_t expr);
    void                check_this_validity(node::pointer_t expr);
    bool                check_unique_functions(node::pointer_t function_node, node::pointer_t class_node, bool const all_levels);
    void                class_directive(node::pointer_t & class_node);
    node::pointer_t     class_of_member(node::pointer_t parent);
    void                comma_operator(node::pointer_t & expr);
    bool                compare_parameters(node::pointer_t & lfunction, node::pointer_t & rfunction);
    void                declare_class(node::pointer_t class_node);
    void                default_directive(node::pointer_t & default_node);
    bool                define_function_type(node::pointer_t func);
    void                directive(node::pointer_t & directive);
    node::pointer_t     directive_list(node::pointer_t directive_list, bool top_list = false);
    void                do_directive(node::pointer_t & do_node);
    void                enum_directive(node::pointer_t & enum_node);
    void                expression(node::pointer_t expr, node::pointer_t params = node::pointer_t());
    bool                expression_new(node::pointer_t expr);
    void                extend_class(node::pointer_t class_node, bool const extend, node::pointer_t extend_name);
    void                finally(node::pointer_t & finally_node);
    bool                find_any_field(node::pointer_t link, node::pointer_t field, node::pointer_t & resolution, node::pointer_t params, node::pointer_t all_matches, int const search_flags);
    depth_t             find_class(node::pointer_t class_type, node::pointer_t type, depth_t depth);
    bool                find_external_package(node::pointer_t import, std::string const & name, node::pointer_t & /*out*/ program_node);
    bool                find_field(node::pointer_t link, node::pointer_t field, node::pointer_t& resolution, node::pointer_t params, node::pointer_t all_matches, int const search_flags);
    bool                find_final_functions(node::pointer_t & function, node::pointer_t & super);
    bool                find_in_extends(node::pointer_t link, node::pointer_t field, node::pointer_t & resolution, node::pointer_t params, node::pointer_t all_matches, int const search_flags);
    void                find_labels(node::pointer_t function, node::pointer_t node);
    bool                find_member(node::pointer_t member, node::pointer_t & /*out*/ resolution, node::pointer_t params, int search_flags);
    bool                find_overloaded_function(node::pointer_t class_node, node::pointer_t function);
    node::pointer_t     find_package(node::pointer_t list, std::string const & name);
    bool                find_package_item(node::pointer_t program, node::pointer_t import, node::pointer_t & /*out*/ resolution, std::string const & name, node::pointer_t params, int const search_flags);
    void                for_directive(node::pointer_t & for_node);
    bool                funcs_name(node::pointer_t resolution, node::pointer_t all_matches);
    void                function(node::pointer_t function_node);
    bool                get_attribute(node::pointer_t node, attribute_t const a);
    unsigned long       get_attributes(node::pointer_t & node);
    search_error_t      get_err_flags() const { return f_err_flags; }
    void                goto_directive(node::pointer_t & goto_node);
    bool                has_abstract_functions(node::pointer_t class_node, node::pointer_t list, node::pointer_t & /*out*/ func);
    void                identifier_to_attrs(node::pointer_t node, node::pointer_t a);
    void                if_directive(node::pointer_t & if_node);
    void                import(node::pointer_t & import);
    bool                is_constructor(node::pointer_t func, node::pointer_t & the_class);
    bool                is_derived_from(node::pointer_t derived_class, node::pointer_t super_class);
    bool                is_dynamic_class(node::pointer_t class_node);
    bool                is_function_abstract(node::pointer_t function);
    bool                is_function_overloaded(node::pointer_t class_node, node::pointer_t function);
    void                link_type(node::pointer_t type);
    depth_t             match_type(node::pointer_t t1, node::pointer_t t2);
    void                node_to_attrs(node::pointer_t node, node::pointer_t a);
    void                object_literal(node::pointer_t expr);
    void                parameters(node::pointer_t parameters_node);
    void                prepare_attributes(node::pointer_t node);
    void                print_search_errors(const node::pointer_t name);
    void                program(node::pointer_t program_node);
    bool                replace_constant_variable(node::pointer_t & /*in,out*/ replace, node::pointer_t resolution);
    bool                resolve_call(node::pointer_t call);
    bool                resolve_operator(node::pointer_t type, node::pointer_t id, node::pointer_t & resolution, node::pointer_t params);
    bool                resolve_field(node::pointer_t object, node::pointer_t field, node::pointer_t& resolution, node::pointer_t params, node::pointer_t all_matches, int const search_flags);
    void                resolve_member(node::pointer_t expr, node::pointer_t params, int const search_flags);
    bool                resolve_name(node::pointer_t list, node::pointer_t id, node::pointer_t & /*out*/ resolution, node::pointer_t params, node::pointer_t all_matches, int const search_flags);
    node::pointer_t     return_directive(node::pointer_t return_node);
    bool                select_best_func(node::pointer_t matches, node::pointer_t & /*out*/ resolution);
    void                set_err_flags(search_error_t flags) { f_err_flags = flags; }
    bool                special_identifier(node::pointer_t expr);
    void                switch_directive(node::pointer_t & switch_node);
    void                throw_directive(node::pointer_t & throw_node);
    void                try_directive(node::pointer_t & try_node);
    void                type_expr(node::pointer_t expr);
    void                unary_operator(node::pointer_t expr);
    void                use_namespace(node::pointer_t & use_namespace_node);
    void                var(node::pointer_t var_node);
    void                variable(node::pointer_t variable_node, bool side_effects_only);
    void                variable_to_attrs(node::pointer_t variable_node, node::pointer_t var);
    void                while_directive(node::pointer_t & while_node);
    void                with(node::pointer_t & with_node);

    time_t                      f_time = 0;                     // time when the compiler is created, see expression values such as __TIME__
    options::pointer_t          f_options = options::pointer_t();
    node::pointer_t             f_program = node::pointer_t();
    bool                        f_result_found = false;         // in a user script, the last expression was found
    input_retriever::pointer_t  f_input_retriever = input_retriever::pointer_t();
    search_error_t              f_err_flags = 0;                // when searching a name and it doesn't get resolve, emit these errors
    node::pointer_t             f_scope = node::pointer_t();    // with() and use namespace list
    module_map_t                f_modules = module_map_t();     // already loaded files (external modules)
};



} // namespace as2js
// vim: ts=4 sw=4 et
