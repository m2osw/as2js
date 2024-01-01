// Copyright (c) 2011-2024  Made to Order Software Corp.  All Rights Reserved
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
#include    "catch_main.h"

#include    "tests/compiler_data/attr_native_class.h"


// as2js
//
#include    <as2js/compiler.h>
#include    <as2js/exception.h>
#include    <as2js/json.h>
#include    <as2js/message.h>
#include    <as2js/parser.h>


// snapdev
//
#include    <snapdev/string_replace_many.h>


// C++
//
#include    <algorithm>
#include    <climits>
#include    <cstring>
#include    <iomanip>


// C
//
#include    <unistd.h>
#include    <sys/stat.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{

bool            g_created_files = false;




class input_retriever
    : public as2js::input_retriever
{
public:
    virtual as2js::base_stream::pointer_t retrieve(std::string const & filename) override
    {
        if(filename == "")
        {
        }

        return as2js::base_stream::pointer_t();
    }

};


std::string     g_current_working_directory;


void init_compiler(as2js::compiler & compiler)
{
    // The .rc file cannot be captured by the input retriever
    // so instead we create a file in the current directory

    // setup an input retriever which in most cases just returns nullptr
    //
    compiler.set_input_retriever(std::make_shared<input_retriever>());
}


void init_rc()
{
    g_created_files = true;

    // we recreate because in the clean up we may end up deleting that
    // folder (even though it's already created by the catch_db_init()
    // function which happens before this call)
    //
    if(mkdir("as2js", 0700) != 0)
    {
        if(errno != EEXIST)
        {
            CATCH_REQUIRE(!"could not create directory as2js");
        }
        // else -- we already created it, that's fine
    }

    // The .rc file cannot be captured by the input retriever
    // so instead we create a file in the current directory
    //
    std::string const safe_cwd(snapdev::string_replace_many(
                              g_current_working_directory
                            , { { "'", "\\'" } }));
    std::ofstream out("as2js/as2js.rc");
    out << "// rc test file\n"
           "{\n"
           "  'scripts': '" << SNAP_CATCH2_NAMESPACE::g_source_dir() << "/scripts',\n"
           "  'db': '" << safe_cwd << "/test.db',\n"
           "  'temporary_variable_name': '@temp$'\n"
           "}\n";

    CATCH_REQUIRE(!!out);
}




}
// no name namespace


namespace SNAP_CATCH2_NAMESPACE
{



} // namespace SNAP_CATCH2_NAMESPACE





CATCH_TEST_CASE("compiler_attributes_inherited", "[compiler][valid]")
{
    CATCH_START_SECTION("compiler_attributes_inherited: simple native class with a function operator")
    {
        // get source code
        //
        std::string const program_source(as2js_tests::attr_native_class, as2js_tests::attr_native_class_size);

        // prepare input stream
        //
        as2js::input_stream<std::stringstream>::pointer_t prog_text(std::make_shared<as2js::input_stream<std::stringstream>>());
        prog_text->get_position().set_filename("tests/compiler_data/attr_native_class.ajs");
        *prog_text << program_source;

        // parse the input
        //
        as2js::options::pointer_t options(std::make_shared<as2js::options>());
        as2js::parser::pointer_t parser(std::make_shared<as2js::parser>(prog_text, options));
        init_rc();
        as2js::node::pointer_t root(parser->parse());

        // run the compiler
        //
        as2js::compiler compiler(options);
        CATCH_REQUIRE(compiler.compile(root) == 0);

        // find nodes of interest and verify they are or not marked with the
        // "native" flag as expected
        //
//std::cerr << "--- resulting node tree is:\n" << *root << "\n";
        as2js::node::pointer_t func(root->find_descendent(
              as2js::node_t::NODE_FUNCTION
            , [](as2js::node::pointer_t n)
            {
                return n->get_string() == "+";
            }));
        CATCH_REQUIRE(func != nullptr);

        as2js::node::pointer_t add(root->find_descendent(as2js::node_t::NODE_ADD, nullptr));
        CATCH_REQUIRE(add->get_attribute(as2js::attribute_t::NODE_ATTR_NATIVE));

        as2js::node::pointer_t product(root->find_descendent(as2js::node_t::NODE_IDENTIFIER,
            [](as2js::node::pointer_t n)
            {
                return n->get_string() == "*";
            }));
        CATCH_REQUIRE(!product->get_attribute(as2js::attribute_t::NODE_ATTR_NATIVE));

        as2js::node::pointer_t member(product->get_parent());
        CATCH_REQUIRE(member->get_type() == as2js::node_t::NODE_MEMBER);
        CATCH_REQUIRE(!member->get_attribute(as2js::attribute_t::NODE_ATTR_NATIVE));

        as2js::node::pointer_t call(member->get_parent());
        CATCH_REQUIRE(call->get_type() == as2js::node_t::NODE_CALL);
        CATCH_REQUIRE_FALSE(call->get_attribute(as2js::attribute_t::NODE_ATTR_NATIVE));

        as2js::node::pointer_t assignment(call->get_parent());
        as2js::node::pointer_t optimized_assignment(assignment->get_parent()->find_next_child(assignment, as2js::node_t::NODE_ASSIGNMENT));
        as2js::node::pointer_t identifier(optimized_assignment->get_child(0));
        CATCH_REQUIRE(identifier->get_type() == as2js::node_t::NODE_IDENTIFIER);
        CATCH_REQUIRE(identifier->get_string() == "e");
        as2js::node::pointer_t integer(optimized_assignment->get_child(1));
        CATCH_REQUIRE(integer->get_type() == as2js::node_t::NODE_INTEGER);
        CATCH_REQUIRE(integer->get_integer().get() == 76 * 12);
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et
