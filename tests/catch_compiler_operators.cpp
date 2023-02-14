// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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

#include    "tests/compiler_data/class_all_operators_overload.h"


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



struct result_t {
    as2js::node_t       f_type = as2js::node_t::NODE_UNKNOWN;
    char const * const  f_call_instance = nullptr;
    char const * const  f_call_type = nullptr;
};


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
constexpr result_t const g_expected_results[] =
{
    {   // ++a
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "++x",
        .f_call_type = "OperatorClass",
    },
    {   // --a
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "--x",
        .f_call_type = "OperatorClass",
    },
    {   // a := -b
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "-",
        .f_call_type = "OperatorClass",
    },
    {   // a := +b
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "+",
        .f_call_type = "OperatorClass",
    },
    {   // a := !b
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "!",
        .f_call_type = "Boolean",
    },
    {   // a := ~b
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "~",
        .f_call_type = "OperatorClass",
    },
    {   // a++
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "x++",
        .f_call_type = "OperatorClass",
    },
    {   // a++
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "x--",
        .f_call_type = "OperatorClass",
    },
    {   // a := b()
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "()",
        .f_call_type = "OperatorClass",
    },
    {   // a := b(c)
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "()",
        .f_call_type = "OperatorClass",
    },
    {   // a := b(-33.57)
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "()",
        .f_call_type = "OperatorClass",
    },
    {   // a := b("param1")
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "()",
        .f_call_type = "OperatorClass",
    },
    {   // a := b(15, "param2", c)
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "()",
        .f_call_type = "Boolean",
    },
    {   // a := b[1]
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "[]",
        .f_call_type = "OperatorClass",
    },
    {   // a := b["index"]
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "[]",
        .f_call_type = "OperatorClass",
    },
    {   // a := b ** c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "**",
        .f_call_type = "OperatorClass",
    },
    {   // a := b ~= /magic/
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "~=",
        .f_call_type = "Boolean",
    },
    {   // a := b ~! /magic/
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "~!",
        .f_call_type = "Boolean",
    },
    {   // a := b * c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "*",
        .f_call_type = "OperatorClass",
    },
    {   // a := b / c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "/",
        .f_call_type = "OperatorClass",
    },
    {   // a := b % c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "%",
        .f_call_type = "OperatorClass",
    },
    {   // a := b + c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "+",
        .f_call_type = "OperatorClass",
    },
    {   // a := b - c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "-",
        .f_call_type = "OperatorClass",
    },
    {   // a := b << 3
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "<<",
        .f_call_type = "OperatorClass",
    },
    {   // a := b >> 3
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = ">>",
        .f_call_type = "OperatorClass",
    },
    {   // a := b >>> 3
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = ">>>",
        .f_call_type = "OperatorClass",
    },
    {   // a := b <% 3
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "<%",
        .f_call_type = "OperatorClass",
    },
    {   // a := b >% 3
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = ">%",
        .f_call_type = "OperatorClass",
    },
    {   // a := b < c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "<",
        .f_call_type = "Boolean",
    },
    {   // a := b <= c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "<=",
        .f_call_type = "Boolean",
    },
    {   // a := b > c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = ">",
        .f_call_type = "Boolean",
    },
    {   // a := b >= c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = ">=",
        .f_call_type = "Boolean",
    },
    {   // a := b == c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "==",
        .f_call_type = "Boolean",
    },
    {   // a := b === c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "===",
        .f_call_type = "Boolean",
    },
    {   // a := b ≈ c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "≈",
        .f_call_type = "Boolean",
    },
    {   // a := b != c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "!=",
        .f_call_type = "Boolean",
    },
    {   // a := b !== c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "!==",
        .f_call_type = "Boolean",
    },
    {   // a := b <=> c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "<=>",
        .f_call_type = "CompareResult",
    },
    {   // a := b ~~ c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "~~",
        .f_call_type = "Boolean",
    },
    {   // a := b & c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "&",
        .f_call_type = "OperatorClass",
    },
    {   // a := b ^ c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "^",
        .f_call_type = "OperatorClass",
    },
    {   // a := b | c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "|",
        .f_call_type = "OperatorClass",
    },
    {   // a := b && c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "&&",
        .f_call_type = "OperatorClass",
    },
    {   // a := b ^^ c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "^^",
        .f_call_type = "OperatorClass",
    },
    {   // a := b || c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "||",
        .f_call_type = "OperatorClass",
    },
    {   // a := b <? c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = "<?",
        .f_call_type = "OperatorClass",
    },
    {   // a := b >? c
        .f_type = as2js::node_t::NODE_ASSIGNMENT,
        .f_call_instance = ">?",
        .f_call_type = "OperatorClass",
    },
    {   // a += b
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "+=",
        .f_call_type = "OperatorClass",
    },
    {   // a &= b
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "&=",
        .f_call_type = "OperatorClass",
    },
    {   // a |= b
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "|=",
        .f_call_type = "OperatorClass",
    },
    {   // a ^= b
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "^=",
        .f_call_type = "OperatorClass",
    },
    {   // a /= b
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "/=",
        .f_call_type = "OperatorClass",
    },
    {   // a &&= b
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "&&=",
        .f_call_type = "OperatorClass",
    },
    {   // a ||= b
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "||=",
        .f_call_type = "OperatorClass",
    },
    {   // a ^^= b
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "^^=",
        .f_call_type = "OperatorClass",
    },
    {   // a >?= b
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = ">?=",
        .f_call_type = "OperatorClass",
    },
    {   // a <?= b
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "<?=",
        .f_call_type = "OperatorClass",
    },
    {   // a %= b
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "%=",
        .f_call_type = "OperatorClass",
    },
    {   // a *= b
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "*=",
        .f_call_type = "OperatorClass",
    },
    {   // a **= b
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "**=",
        .f_call_type = "OperatorClass",
    },
    {   // a <%= 3
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "<%=",
        .f_call_type = "OperatorClass",
    },
    {   // a >%= 3
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = ">%=",
        .f_call_type = "OperatorClass",
    },
    {   // a <<= 3
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "<<=",
        .f_call_type = "OperatorClass",
    },
    {   // a >>= 3
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = ">>=",
        .f_call_type = "OperatorClass",
    },
    {   // a >>>= 3
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = ">>>=",
        .f_call_type = "OperatorClass",
    },
    {   // a -= b
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = "-=",
        .f_call_type = "OperatorClass",
    },
    {   // a := b, c
        .f_type = as2js::node_t::NODE_CALL,
        .f_call_instance = ",",
        .f_call_type = "OperatorClass",
    },
};
#pragma GCC diagnostic pop

constexpr std::size_t const g_expected_results_size = std::size(g_expected_results);




bool            g_created_files = false;




//class input_retriever
//    : public as2js::input_retriever
//{
//public:
//    virtual as2js::base_stream::pointer_t retrieve(std::string const & filename) override
//    {
//        if(filename == "")
//        {
//        }
//
//        return as2js::base_stream::pointer_t();
//    }
//
//};


std::string     g_current_working_directory;


//void init_compiler(as2js::compiler & compiler)
//{
//    // The .rc file cannot be captured by the input retriever
//    // so instead we create a file in the current directory
//
//    // setup an input retriever which in most cases just returns nullptr
//    //
//    compiler.set_input_retriever(std::make_shared<input_retriever>());
//}


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





CATCH_TEST_CASE("compiler_all_operators", "[compiler][valid]")
{
    CATCH_START_SECTION("compiler_all_operators: user class with all possible operators")
    {
        // get source code
        //
        std::string const program_source(as2js_tests::class_all_operators_overload, as2js_tests::class_all_operators_overload_size);

        // prepare input stream
        //
        as2js::input_stream<std::stringstream>::pointer_t prog_text(std::make_shared<as2js::input_stream<std::stringstream>>());
        prog_text->get_position().set_filename("tests/compiler_data/class_all_operators_overload.ajs");
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
std::cerr << "--- start compiling operators:\n" << *root << "\n";
        CATCH_REQUIRE(compiler.compile(root) == 0);

        // find nodes of interest and verify they are or not marked with the
        // "native" flag as expected
        //
std::cerr << "--- resulting node tree is:\n" << *root << "\n";
        as2js::node::pointer_t operator_class(root->find_descendent(
              as2js::node_t::NODE_CLASS
            , [](as2js::node::pointer_t n)
            {
                return n->get_string() == "OperatorClass";
            }));
        CATCH_REQUIRE(operator_class != nullptr);

        as2js::node::pointer_t call;
        as2js::node::pointer_t assignment;
        for(std::size_t i(0); i < g_expected_results_size; ++i)
        {
            if(i == 0)
            {
                call = root->find_descendent(
                          g_expected_results[i].f_type
                        , [&operator_class](as2js::node::pointer_t n)
                        {
                            return n->get_type_node() == operator_class;
                        });
            }
            else if(assignment != nullptr)
            {
                call = assignment->get_parent()->find_next_child(assignment, g_expected_results[i].f_type);
            }
            else
            {
                call = call->get_parent()->find_next_child(call, g_expected_results[i].f_type);
            }
            CATCH_REQUIRE(call != nullptr);
std::cerr << i + 1 << ". checking " << g_expected_results[i].f_call_instance << " with call at " << call.get() << "\n";

            if(g_expected_results[i].f_type != as2js::node_t::NODE_CALL)
            {
                assignment = call;
std::cerr << "--- got an ASSIGNMENT which looks like this:\n" << *call << "\n";
                call = assignment->find_descendent(
                          as2js::node_t::NODE_CALL
                        , [](as2js::node::pointer_t)
                        {
                            // some return a Boolean, not the OperatorClass
                            //return n->get_type_node() == operator_class;
                            return true;
                        });
                CATCH_REQUIRE(call != nullptr);

                CATCH_REQUIRE(assignment->get_type_node() == operator_class);
            }
            else
            {
                assignment.reset();
std::cerr << "--- got a CALL which looks like this:\n" << *call << "\n";
            }

            CATCH_REQUIRE_FALSE(call->get_attribute(as2js::attribute_t::NODE_ATTR_NATIVE));

            CATCH_REQUIRE(call->get_instance() != nullptr);
            CATCH_REQUIRE(call->get_instance()->get_string() == g_expected_results[i].f_call_instance);

            as2js::node::pointer_t check_type(call->get_type_node());
            CATCH_REQUIRE(check_type != nullptr);
            CATCH_REQUIRE(check_type->get_string() == g_expected_results[i].f_call_type);

            // the return type is generally operator_class, but a few functions
            // return something else such as Boolean
        }

        // if someone was to make the expected results array empty, this
        // would be triggered, otherwise it cannot happen
        //
        CATCH_REQUIRE(call != nullptr);

        // no more calls or we have a problem in our test or the library
        //
        call = call->get_parent()->find_next_child(call, as2js::node_t::NODE_CALL);
        if(g_expected_results[g_expected_results_size - 1].f_type == as2js::node_t::NODE_CALL)
        {
            CATCH_REQUIRE(call != nullptr);
            as2js::node::pointer_t id(call->find_descendent(
                      as2js::node_t::NODE_IDENTIFIER
                    , [](as2js::node::pointer_t n)
                    {
                        return n->get_string() == "console";
                    }));
            CATCH_REQUIRE(id != nullptr);
        }
        else
        {
            CATCH_REQUIRE(call == nullptr);
        }

        //as2js::node::pointer_t add(root->find_descendent(as2js::node_t::NODE_ADD, nullptr));
        //CATCH_REQUIRE(add != nullptr);

        //as2js::node::pointer_t product(root->find_descendent(as2js::node_t::NODE_IDENTIFIER,
        //    [](as2js::node::pointer_t n)
        //    {
        //        return n->get_string() == "*";
        //    }));
        //CATCH_REQUIRE(product != nullptr);
        //CATCH_REQUIRE(!product->get_attribute(as2js::attribute_t::NODE_ATTR_NATIVE));

        //as2js::node::pointer_t member(product->get_parent());
        //CATCH_REQUIRE(member->get_type() == as2js::node_t::NODE_MEMBER);
        //CATCH_REQUIRE(!member->get_attribute(as2js::attribute_t::NODE_ATTR_NATIVE));

        //as2js::node::pointer_t call(member->get_parent());
        //CATCH_REQUIRE(call->get_type() == as2js::node_t::NODE_CALL);
        //CATCH_REQUIRE(!call->get_attribute(as2js::attribute_t::NODE_ATTR_NATIVE));
    }
    CATCH_END_SECTION()
}



// vim: ts=4 sw=4 et
