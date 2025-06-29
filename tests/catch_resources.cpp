// Copyright (c) 2011-2025  Made to Order Software Corp.  All Rights Reserved
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

// as2js
//
// the resources.h is private hence the use of "..."
//
#include    "as2js/file/resources.h"

#include    <as2js/exception.h>
#include    <as2js/message.h>



// self
//
#include    "catch_main.h"


// snapdev
//
#include    <snapdev/mkdir_p.h>
#include    <snapdev/safe_setenv.h>


// C++
//
#include    <cstring>


// C
//
#include    <sys/stat.h>
#include    <unistd.h>


// last include
//
#include    <snapdev/poison.h>




namespace
{


class test_callback
    : public as2js::message_callback
{
public:
    test_callback()
    {
        as2js::set_message_callback(this);
        g_warning_count = as2js::warning_count();
        g_error_count = as2js::error_count();
    }

    ~test_callback()
    {
        // make sure the pointer gets reset!
        as2js::set_message_callback(nullptr);
    }

    // implementation of the output
    virtual void output(as2js::message_level_t message_level, as2js::err_code_t error_code, as2js::position const& pos, std::string const& message)
    {
        if(f_expected.empty())
        {
            std::cerr << "\nfilename = " << pos.get_filename() << "\n";
            std::cerr << "msg = " << message << "\n";
            std::cerr << "page = " << pos.get_page() << "\n";
            std::cerr << "line = " << pos.get_line() << "\n";
            std::cerr << "error_code = " << static_cast<int>(error_code) << "\n";
        }

        CATCH_REQUIRE(!f_expected.empty());

//std::cerr << "filename = " << pos.get_filename() << " / " << f_expected[0].f_pos.get_filename() << "\n";
//std::cerr << "msg = " << message << " / " << f_expected[0].f_message << "\n";
//std::cerr << "page = " << pos.get_page() << " / " << f_expected[0].f_pos.get_page() << "\n";
//std::cerr << "line = " << pos.get_line() << " / " << f_expected[0].f_pos.get_line() << "\n";
//std::cerr << "error_code = " << static_cast<int>(error_code) << " / " << static_cast<int>(f_expected[0].f_error_code) << "\n";

        CATCH_REQUIRE(f_expected[0].f_call);
        CATCH_REQUIRE(message_level == f_expected[0].f_message_level);
        CATCH_REQUIRE(error_code == f_expected[0].f_error_code);
        CATCH_REQUIRE(pos.get_filename() == f_expected[0].f_pos.get_filename());
        CATCH_REQUIRE(pos.get_function() == f_expected[0].f_pos.get_function());
        CATCH_REQUIRE(pos.get_page() == f_expected[0].f_pos.get_page());
        CATCH_REQUIRE(pos.get_page_line() == f_expected[0].f_pos.get_page_line());
        CATCH_REQUIRE(pos.get_paragraph() == f_expected[0].f_pos.get_paragraph());
        CATCH_REQUIRE(pos.get_line() == f_expected[0].f_pos.get_line());
        CATCH_REQUIRE(message == f_expected[0].f_message);

        if(message_level == as2js::message_level_t::MESSAGE_LEVEL_WARNING)
        {
            ++g_warning_count;
            CATCH_REQUIRE(g_warning_count == as2js::warning_count());
        }

        if(message_level == as2js::message_level_t::MESSAGE_LEVEL_FATAL
        || message_level == as2js::message_level_t::MESSAGE_LEVEL_ERROR)
        {
            ++g_error_count;
//std::cerr << "error: " << g_error_count << " / " << as2js::error_count() << "\n";
            CATCH_REQUIRE(g_error_count == as2js::error_count());
        }

        f_expected.erase(f_expected.begin());
    }

    void got_called()
    {
        if(!f_expected.empty())
        {
            std::cerr << "\n*** STILL EXPECTED: ***\n";
            std::cerr << "filename = " << f_expected[0].f_pos.get_filename() << "\n";
            std::cerr << "msg = " << f_expected[0].f_message << "\n";
            std::cerr << "page = " << f_expected[0].f_pos.get_page() << "\n";
            std::cerr << "error_code = " << static_cast<int>(f_expected[0].f_error_code) << "\n";
        }
        CATCH_REQUIRE(f_expected.empty());
    }

    struct expected_t
    {
        bool                        f_call = true;
        as2js::message_level_t      f_message_level = as2js::message_level_t::MESSAGE_LEVEL_OFF;
        as2js::err_code_t           f_error_code = as2js::err_code_t::AS_ERR_NONE;
        as2js::position             f_pos = as2js::position();
        std::string                 f_message = std::string(); // UTF-8 string
    };

    std::vector<expected_t>     f_expected = std::vector<expected_t>();

    static int32_t              g_warning_count;
    static int32_t              g_error_count;
};

int32_t   test_callback::g_warning_count = 0;
int32_t   test_callback::g_error_count = 0;

int32_t   g_empty_home_too_late = 0;

}
// no name namespace




namespace SNAP_CATCH2_NAMESPACE
{



int catch_rc_init()
{
    // verify that this user does not have existing rc files because
    // that can interfer with the tests! (and we do not want to delete
    // those under their feet)

    // AS2JS_RC variable must not exist
    //
    if(getenv("AS2JS_RC") != nullptr)
    {
        std::cerr << "error: the \"AS2JS_RC\" environment variable is defined; please make sure you want to run this test on this system and unset that variable before doing so.\n";
        return 1;
    }

    // local file
    //
    struct stat st;
    if(stat("as2js/as2js.rc", &st) != -1)
    {
        std::cerr << "error: file \"as2js/as2js.rc\" already exists; please check it out to make sure you can delete it and try running the test again.\n";
        return 1;
    }

    // user defined .config file
    //
    if(getenv("HOME") == nullptr)
    {
        std::cerr << "error: the \"HOME\" environment variable is expected to be defined.\n";
        return 1;
    }
    std::string const home(getenv("HOME"));
    std::string config(home);
    config += "/.config/as2js/as2js.rc";
    if(stat(config.c_str(), &st) != -1)
    {
        std::cerr << "error: file \""
            << config
            << "\" already exists; please check it out to make sure you can delete it and try running the test again.\n";
        return 1;
    }

    // system defined configuration file
    //
    if(stat("/etc/as2js/as2js.rc", &st) != -1)
    {
        std::cerr << "error: file \""
            << "/etc/as2js/as2js.rc"
            << "\" already exists; please check it out to make sure you can delete it and try running the test again.\n";
        return 1;
    }

    // create the local as2js directory now
    //
    if(mkdir("as2js", 0700) != 0)
    {
        int const e(errno);
        std::cerr << "error: could not create directory \""
            << home
            << "as2js"
            << "\". (errno: "
            << e
            << ", "
            << strerror(e)
            << ")\n";
        return 1;
    }

    return 0;
}



} // namespace SNAP_CATCH2_NAMESPACE


CATCH_TEST_CASE("resources_basics", "[resources][file]")
{
    CATCH_START_SECTION("resources_basics: check paths & filenames")
    {
        // this test is not going to work if the get_home() function was
        // already called with an empty HOME variable...
        //
        if(g_empty_home_too_late == 2)
        {
            std::cout << " --- test_empty_home() not run, the other rc unit tests are not compatible with this test --- ";
            return;
        }

        g_empty_home_too_late = 1;

        {
            // test the get_home()
            //
            std::string const home(getenv("HOME"));
            std::string rc_home(as2js::resources::get_home());
            CATCH_REQUIRE(rc_home == home);

            // verify that changing the variable after the first call returns
            // the first value...
            //
            snapdev::transparent_setenv safe_home("HOME", "/got/changed/now");
            rc_home = as2js::resources::get_home();
            CATCH_REQUIRE(rc_home == home);
        } // restore original HOME

        // without the as2js/scripts sub-directory, we get nothing
        {
            as2js::resources rc;
            as2js::resources::script_paths_t paths(rc.get_scripts());
            CATCH_REQUIRE(paths.empty());
            CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
            CATCH_REQUIRE(rc.get_temporary_variable_name() == "@temp");
        }

        {
            //std::string const home(getenv("HOME"));
            //CATCH_REQUIRE(snapdev::mkdir_p(home + "/as2js/scripts", false, 0700) == 0);
            CATCH_REQUIRE(snapdev::mkdir_p("as2js/scripts", false, 0700) == 0);
            char * cwd(get_current_dir_name());
            as2js::resources rc;
            as2js::resources::script_paths_t paths(rc.get_scripts());
            CATCH_REQUIRE(paths.size() == 1);
            CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/as2js/scripts");
            CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
            CATCH_REQUIRE(rc.get_temporary_variable_name() == "@temp");
            free(cwd);
        }

        {
            as2js::resources rc;

            test_callback tc;

            test_callback::expected_t expected1;
            expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
            expected1.f_error_code = as2js::err_code_t::AS_ERR_INSTALLATION;
            expected1.f_pos.set_filename("unknown-file");
            expected1.f_pos.set_function("unknown-func");
            expected1.f_message = "cannot find the \"as2js.rc\" file; the system default is usually put in \"/etc/as2js/as2js.rc\".";
            tc.f_expected.push_back(expected1);

            CATCH_REQUIRE_THROWS_MATCHES(
                  rc.init(false)
                , as2js::as2js_exit
                , Catch::Matchers::ExceptionMessage(
                          "as2js_exception: cannot find the \"as2js.rc\" file; the system default is usually put in \"/etc/as2js/as2js.rc\"."));
            tc.got_called();

            rc.init(true);

            as2js::resources::script_paths_t paths(rc.get_scripts());
            CATCH_REQUIRE(paths.size() == 1);
            char * cwd(get_current_dir_name());
            CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/as2js/scripts");
            free(cwd);
            CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("resources_load_from_var", "[resources][config][file][variable]")
{
    CATCH_START_SECTION("resources_load_from_var: NULL value")
    {
        // this test is not going to work if the get_home() function was
        // already called with an empty HOME variable...
        if(g_empty_home_too_late == 2)
        {
            std::cout << " --- test_empty_home() not run, the other rc unit tests are not compatible with this test --- ";
            return;
        }

        g_empty_home_too_late = 1;

        // just in case it failed before...
        //
        unlink("as2js.rc");

        {
            setenv("AS2JS_RC", ".", 1);

            test_callback tc;

            test_callback::expected_t expected1;
            expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
            expected1.f_error_code = as2js::err_code_t::AS_ERR_INSTALLATION;
            expected1.f_pos.set_filename("unknown-file");
            expected1.f_pos.set_function("unknown-func");
            expected1.f_message = "cannot find the \"as2js.rc\" file; the system default is usually put in \"/etc/as2js/as2js.rc\".";
            tc.f_expected.push_back(expected1);

            as2js::resources rc;
            CATCH_REQUIRE_THROWS_MATCHES(
                  rc.init(false)
                , as2js::as2js_exit
                , Catch::Matchers::ExceptionMessage(
                          "as2js_exception: cannot find the \"as2js.rc\" file; the system default is usually put in \"/etc/as2js/as2js.rc\"."));
            tc.got_called();

            {
                // create an .rc file
                CATCH_REQUIRE(snapdev::mkdir_p("the/script", false, 0700) == 0);
                {
                    std::ofstream rc_file;
                    rc_file.open("as2js.rc");
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "{\n"
                            << "  'scripts': 'the/script',\n"
                            << "  'db': 'that/db',\n"
                            << "  'temporary_variable_name': '@temp$'\n"
                            << "}\n";
                }

                rc.init(true);
                unlink("as2js.rc");

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/the/script");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "that/db");
                CATCH_REQUIRE(rc.get_temporary_variable_name() == "@temp$");
            }

            {
                // create an .rc file, without scripts
                {
                    std::ofstream rc_file;
                    rc_file.open("as2js.rc");
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "{\n"
                            << "  'db': 'that/db'\n"
                            << "}\n";
                }

                rc.init(true);
                unlink("as2js.rc");

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/as2js/scripts");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "that/db");
                CATCH_REQUIRE(rc.get_temporary_variable_name() == "@temp");
            }

            {
                // create an .rc file, without scripts
                CATCH_REQUIRE(snapdev::mkdir_p("the/script", false, 0700) == 0);
                {
                    std::ofstream rc_file;
                    rc_file.open("as2js.rc");
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "{\n"
                            << "  'scripts': 'the/script'\n"
                            << "}\n";
                }

                rc.init(true);
                unlink("as2js.rc");

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/the/script");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
                CATCH_REQUIRE(rc.get_temporary_variable_name() == "@temp");
            }

            {
                // create an .rc file, with just the temporary variable name
                {
                    std::ofstream rc_file;
                    rc_file.open("as2js.rc");
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "{\n"
                            << "  \"temporary_variable_name\": \"what about validity of the value? -- we on purpose use @ because it is not valid in identifiers\"\n"
                            << "}\n";
                }

                rc.init(true);
                unlink("as2js.rc");

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/as2js/scripts");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
                CATCH_REQUIRE(rc.get_temporary_variable_name() == "what about validity of the value? -- we on purpose use @ because it is not valid in identifiers");
            }

            {
                // create an .rc file, without scripts
                {
                    std::ofstream rc_file;
                    rc_file.open("as2js.rc");
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "{\n"
                            << "  'scripts': 123\n"
                            << "}\n";
                }

                test_callback::expected_t expected2;
                expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
                expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_RC;
                expected2.f_pos.set_filename("./as2js.rc");
                expected2.f_pos.set_function("unknown-func");
                expected2.f_pos.new_line();
                expected2.f_pos.new_line();
                expected2.f_pos.new_line();
                expected2.f_message = "a resource file is expected to be an object of string elements.";
                tc.f_expected.push_back(expected2);

                CATCH_REQUIRE_THROWS_MATCHES(
                      rc.init(true)
                    , as2js::as2js_exit
                    , Catch::Matchers::ExceptionMessage(
                              "as2js_exception: a resource file is expected to be an object of string elements."));
                tc.got_called();
                unlink("as2js.rc");

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/as2js/scripts");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
            }

            {
                // create a null .rc file
                {
                    std::ofstream rc_file;
                    rc_file.open("as2js.rc");
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "null\n";
                }

                rc.init(false);
                unlink("as2js.rc");

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/as2js/scripts");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
            }

            {
                // create an .rc file, without an object nor null
                {
                    std::ofstream rc_file;
                    rc_file.open("as2js.rc");
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "['scripts', 123]\n";
                }

                test_callback::expected_t expected2;
                expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
                expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_RC;
                expected2.f_pos.set_filename("./as2js.rc");
                expected2.f_pos.set_function("unknown-func");
                expected2.f_pos.new_line();
                expected2.f_message = "./as2js.rc: a resource file (.rc) must be defined as a JSON object, or set to \"null\".";
                tc.f_expected.push_back(expected2);

                CATCH_REQUIRE_THROWS_MATCHES(
                      rc.init(true)
                    , as2js::as2js_exit
                    , Catch::Matchers::ExceptionMessage(
                              "as2js_exception: ./as2js.rc: a resource file (.rc) must be defined as a JSON object, or set to \"null\"."));
                tc.got_called();
                unlink("as2js.rc");

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/as2js/scripts");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
            }

            // test some other directory too
            setenv("AS2JS_RC", "/tmp", 1);

            {
                // create an .rc file
                CATCH_REQUIRE(snapdev::mkdir_p("the/script", false, 0700) == 0);
                {
                    std::ofstream rc_file;
                    rc_file.open("/tmp/as2js.rc");
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "{\n"
                            << "  'scripts': 'the/script',\n"
                            << "  'db': 'that/db'\n"
                            << "}\n";
                }

                rc.init(true);
                unlink("/tmp/as2js.rc");

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/the/script");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "that/db");
            }

            // make sure to delete that before exiting
            //
            unsetenv("AS2JS_RC");
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("resources_load_from_local_config", "[resources][config][file]")
{
    CATCH_START_SECTION("resources_load_from_local_config: check that the local as2js.rc gets picked up")
    {
        // this test is not going to work if the get_home() function was
        // already called with an empty HOME variable...
        if(g_empty_home_too_late == 2)
        {
            std::cout << "--- test_empty_home() not run, the other rc unit tests are not compatible with this test ---\n";
            return;
        }

        g_empty_home_too_late = 1;

        // just in case it failed before...
        //
        static_cast<void>(unlink(".config/as2js.rc"));
        static_cast<void>(rmdir(".config"));

        CATCH_REQUIRE(mkdir(".config", 0700) == 0);

        {
            test_callback tc;

            test_callback::expected_t expected1;
            expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
            expected1.f_error_code = as2js::err_code_t::AS_ERR_INSTALLATION;
            expected1.f_pos.set_filename("unknown-file");
            expected1.f_pos.set_function("unknown-func");
            expected1.f_message = "cannot find the \"as2js.rc\" file; the system default is usually put in \"/etc/as2js/as2js.rc\".";
            tc.f_expected.push_back(expected1);

            as2js::resources rc;
            CATCH_REQUIRE_THROWS_MATCHES(
                  rc.init(false)
                , as2js::as2js_exit
                , Catch::Matchers::ExceptionMessage(
                          "as2js_exception: cannot find the \"as2js.rc\" file; the system default is usually put in \"/etc/as2js/as2js.rc\"."));
            tc.got_called();

            {
                // create an .rc file
                {
                    std::ofstream rc_file;
                    rc_file.open("as2js/as2js.rc");
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "{\n"
                            << "  'scripts': 'the/script',\n"
                            << "  'db': 'that/db'\n"
                            << "}\n";
                }

                rc.init(true);
                unlink("as2js/as2js.rc");

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/the/script");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "that/db");
            }

            {
                // create an .rc file, without scripts
                {
                    std::ofstream rc_file;
                    rc_file.open("as2js/as2js.rc");
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "{\n"
                            << "  'db': 'that/db'\n"
                            << "}\n";
                }

                rc.init(true);
                unlink("as2js/as2js.rc");

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/as2js/scripts");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "that/db");
            }

            {
                // create an .rc file, without scripts
                {
                    std::ofstream rc_file;
                    rc_file.open("as2js/as2js.rc");
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "{\n"
                            << "  'scripts': 'the/script'\n"
                            << "}\n";
                }

                rc.init(true);
                unlink("as2js/as2js.rc");

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/the/script");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
            }

            {
                // create an .rc file, without scripts
                {
                    std::ofstream rc_file;
                    rc_file.open("as2js/as2js.rc");
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "{\n"
                            << "  'scripts': 123\n"
                            << "}\n";
                }

                test_callback::expected_t expected2;
                expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
                expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_RC;
                expected2.f_pos.set_filename("as2js/as2js.rc");
                expected2.f_pos.set_function("unknown-func");
                expected2.f_pos.new_line();
                expected2.f_pos.new_line();
                expected2.f_pos.new_line();
                expected2.f_message = "a resource file is expected to be an object of string elements.";
                tc.f_expected.push_back(expected2);

                CATCH_REQUIRE_THROWS_MATCHES(
                      rc.init(true)
                    , as2js::as2js_exit
                    , Catch::Matchers::ExceptionMessage(
                              "as2js_exception: a resource file is expected to be an object of string elements."));
                tc.got_called();
                unlink("as2js/as2js.rc");

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/as2js/scripts");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
            }

            {
                // create a null .rc file
                {
                    std::ofstream rc_file;
                    rc_file.open("as2js/as2js.rc");
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "null\n";
                }

                rc.init(false);
                unlink("as2js/as2js.rc");

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/as2js/scripts");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
            }

            {
                // create an .rc file, without an object nor null
                {
                    std::ofstream rc_file;
                    rc_file.open("as2js/as2js.rc");
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "['scripts', 123]\n";
                }

                test_callback::expected_t expected2;
                expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
                expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_RC;
                expected2.f_pos.set_filename("as2js/as2js.rc");
                expected2.f_pos.set_function("unknown-func");
                expected2.f_pos.new_line();
                expected2.f_message = "as2js/as2js.rc: a resource file (.rc) must be defined as a JSON object, or set to \"null\".";
                tc.f_expected.push_back(expected2);

                CATCH_REQUIRE_THROWS_MATCHES(
                      rc.init(true)
                    , as2js::as2js_exit
                    , Catch::Matchers::ExceptionMessage(
                              "as2js_exception: as2js/as2js.rc: a resource file (.rc) must be defined as a JSON object, or set to \"null\"."));
                tc.got_called();
                unlink("as2js/as2js.rc");

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/as2js/scripts");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
            }
        }

        // delete our temporary .rc file (should already have been deleted)
        unlink("as2js/as2js.rc");

        // if possible get rid of the directory (don't check for errors)
        rmdir("as2js");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("resources_load_from_user_config", "[resources][config][file]")
{
    CATCH_START_SECTION("resources_load_from_user_config: NULL value")
    {
        // this test is not going to work if the get_home() function was
        // already called with an empty HOME variable...
        if(g_empty_home_too_late == 2)
        {
            std::cout << "--- test_empty_home() not run, the other rc unit tests are not compatible with this test ---\n";
            return;
        }

        g_empty_home_too_late = 1;

        std::string const home(getenv("HOME"));

        // create the folders and make sure we clean up any existing .rc file
        // (although it was checked in the setUp() function and thus we should
        // not reach here if the .rc already existed!)
        //
        std::string config(home);
        config += "/.config";
        std::cout << "--- config path \"" << config << "\" ---\n";
        bool del_config(true);
        if(mkdir(config.c_str(), 0700) != 0) // usually this is 0755, but for security we cannot take that risk...
        {
            // if this mkdir() fails, it is because it exists?
            //
            CATCH_REQUIRE(errno == EEXIST);
            del_config = false;
        }
        std::string as2js_conf(config);
        as2js_conf += "/as2js";
        CATCH_REQUIRE(snapdev::mkdir_p(as2js_conf.c_str(), false, 0700) == 0);
        std::string as2js_rc(as2js_conf);
        as2js_rc += "/as2js.rc";
        unlink(as2js_rc.c_str()); // delete that, just in case (the setup verifies that it does not exist)
//system(("ls -lR " + config).c_str());

        {
            test_callback tc;

            test_callback::expected_t expected1;
            expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
            expected1.f_error_code = as2js::err_code_t::AS_ERR_INSTALLATION;
            expected1.f_pos.set_filename("unknown-file");
            expected1.f_pos.set_function("unknown-func");
            expected1.f_message = "cannot find the \"as2js.rc\" file; the system default is usually put in \"/etc/as2js/as2js.rc\".";
            tc.f_expected.push_back(expected1);

            as2js::resources rc;
            CATCH_REQUIRE_THROWS_MATCHES(
                  rc.init(false)
                , as2js::as2js_exit
                , Catch::Matchers::ExceptionMessage(
                          "as2js_exception: cannot find the \"as2js.rc\" file; the system default is usually put in \"/etc/as2js/as2js.rc\"."));
            tc.got_called();

            {
                // create an .rc file
                CATCH_REQUIRE(snapdev::mkdir_p("the/script", false, 0700) == 0);
                CATCH_REQUIRE(snapdev::mkdir_p("another/script", false, 0700) == 0);
                CATCH_REQUIRE(snapdev::mkdir_p("here/script", false, 0700) == 0);
                {
                    std::ofstream rc_file;
                    rc_file.open(as2js_rc.c_str());
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "{\n"
                            << "  'scripts': 'the/script:another/script:here/script',\n"
                            << "  'db': 'that/db'\n"
                            << "}\n";
                }

                rc.init(true);
                unlink(as2js_rc.c_str());

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 3);
                auto it(paths.begin());
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*it == std::string(cwd) + "/the/script");
                ++it;
                CATCH_REQUIRE(*it == std::string(cwd) + "/another/script");
                ++it;
                CATCH_REQUIRE(*it == std::string(cwd) + "/here/script");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "that/db");
            }

            {
                // create an .rc file, without scripts
                {
                    std::ofstream rc_file;
                    rc_file.open(as2js_rc.c_str());
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "{\n"
                            << "  'db': 'that/db'\n"
                            << "}\n";
                }

                rc.init(true);
                unlink(as2js_rc.c_str());

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/as2js/scripts");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "that/db");
            }

            {
                // create an .rc file, without scripts
                {
                    std::ofstream rc_file;
                    rc_file.open(as2js_rc.c_str());
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "{\n"
                            << "  'scripts': 'the/script'\n"
                            << "}\n";
                }

                rc.init(true);
                unlink(as2js_rc.c_str());

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/the/script");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
            }

            {
                // create an .rc file, without scripts
                {
                    std::ofstream rc_file;
                    rc_file.open(as2js_rc.c_str());
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "{\n"
                            << "  'scripts': 123\n"
                            << "}\n";
                }

                test_callback::expected_t expected2;
                expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
                expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_RC;
                expected2.f_pos.set_filename(as2js_rc.c_str());
                expected2.f_pos.set_function("unknown-func");
                expected2.f_pos.new_line();
                expected2.f_pos.new_line();
                expected2.f_pos.new_line();
                expected2.f_message = "a resource file is expected to be an object of string elements.";
                tc.f_expected.push_back(expected2);

                CATCH_REQUIRE_THROWS_MATCHES(
                      rc.init(true)
                    , as2js::as2js_exit
                    , Catch::Matchers::ExceptionMessage(
                              "as2js_exception: a resource file is expected to be an object of string elements."));
                tc.got_called();
                unlink(as2js_rc.c_str());

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/as2js/scripts");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
            }

            {
                // create a null .rc file
                {
                    std::ofstream rc_file;
                    rc_file.open(as2js_rc.c_str());
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "null\n";
                }

                rc.init(false);
                unlink(as2js_rc.c_str());

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                char * cwd(get_current_dir_name());
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/as2js/scripts");
                free(cwd);
                CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
            }

            {
                // create an .rc file, without an object nor null
                {
                    std::ofstream rc_file;
                    rc_file.open(as2js_rc.c_str());
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "['scripts', 123]\n";
                }

                char * cwd(get_current_dir_name());
                test_callback::expected_t expected2;
                expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
                expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_RC;
                expected2.f_pos.set_filename(as2js_rc.c_str());
                expected2.f_pos.set_function("unknown-func");
                expected2.f_pos.new_line();
                expected2.f_message = std::string(cwd) + "/home/.config/as2js/as2js.rc: a resource file (.rc) must be defined as a JSON object, or set to \"null\".";
                tc.f_expected.push_back(expected2);

                CATCH_REQUIRE_THROWS_MATCHES(
                      rc.init(true)
                    , as2js::as2js_exit
                    , Catch::Matchers::ExceptionMessage(
                              "as2js_exception: " + expected2.f_message));
                              //"as2js_exception: a resource file (.rc) must be defined as a JSON object, or set to \"null\"."));
                tc.got_called();
                unlink(as2js_rc.c_str());

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                CATCH_REQUIRE(*paths.begin() == std::string(cwd) + "/as2js/scripts");
                CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
                free(cwd);
            }
        }

        // delete our temporary .rc file (should already have been deleted)
        unlink(as2js_rc.c_str());

        // if possible get rid of the directories (don't check for errors)
        rmdir(as2js_conf.c_str());
        if(del_config)
        {
            rmdir(config.c_str());
        }
    }
    CATCH_END_SECTION()
}


//
// WARNING: this test requires root permissions, it can generally be
//          ignored though because it uses the same process as the
//          user local file in "as2js/as2js.rc"; it is here for
//          completeness in case you absolutely want to prove that
//          works as expected
//
CATCH_TEST_CASE("resources_load_from_system_config", "[resources][config][file]")
{
    CATCH_START_SECTION("resources_load_from_system_config: NULL value")
    {
        if(getuid() != 0)
        {
            std::cout << "--- test_load_from_system_config() requires root access to modify the /etc/as2js directory ---\n";
            CATCH_REQUIRE(getuid() != 0);
            return;
        }

        // this test is not going to work if the get_home() function was
        // already called with an empty HOME variable...
        //
        if(g_empty_home_too_late == 2)
        {
            std::cout << "--- test_empty_home() not run, the other rc unit tests are not compatible with this test ---\n";
            return;
        }

        g_empty_home_too_late = 1;

        // create the folders and make sure we clean up any existing .rc file
        // (although it was checked in the setUp() function and thus we should
        // not reach here if the .rc already existed!)
        std::string as2js_conf("/etc/as2js");
        int const r(mkdir(as2js_conf.c_str(), 0700)); // usually this is 0755, but for security we cannot take that risk...
        if(r != 0)
        {
            // if this mkdir() fails, it is because it exists?
            //
            CATCH_REQUIRE(errno == EEXIST);
        }
        std::string as2js_rc(as2js_conf);
        as2js_rc += "/as2js.rc";
        unlink(as2js_rc.c_str()); // delete that, just in case (the setup verifies that it does not exist)

        {
            test_callback tc;

            test_callback::expected_t expected1;
            expected1.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
            expected1.f_error_code = as2js::err_code_t::AS_ERR_INSTALLATION;
            expected1.f_pos.set_filename("unknown-file");
            expected1.f_pos.set_function("unknown-func");
            expected1.f_message = "cannot find the \"as2js.rc\" file; the system default is usually put in \"/etc/as2js/as2js.rc\".";
            tc.f_expected.push_back(expected1);

            as2js::resources rc;
            CATCH_REQUIRE_THROWS_MATCHES(
                  rc.init(false)
                , as2js::as2js_exit
                , Catch::Matchers::ExceptionMessage(
                          "as2js_exception: cannot find the \"as2js.rc\" file; the system default is usually put in \"/etc/as2js/as2js.rc\"."));
            tc.got_called();

            {
                // create an .rc file
                {
                    std::ofstream rc_file;
                    rc_file.open(as2js_rc.c_str());
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "{\n"
                            << "  'scripts': 'the/script',\n"
                            << "  'db': 'that/db'\n"
                            << "}\n";
                }

                rc.init(true);
                unlink(as2js_rc.c_str());

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                CATCH_REQUIRE(*paths.begin() == "the/script");
                CATCH_REQUIRE(rc.get_db() == "that/db");
            }

            {
                // create an .rc file, without scripts
                {
                    std::ofstream rc_file;
                    rc_file.open(as2js_rc.c_str());
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "{\n"
                            << "  'db': 'that/db'\n"
                            << "}\n";
                }

                rc.init(true);
                unlink(as2js_rc.c_str());

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                CATCH_REQUIRE(*paths.begin() == "as2js/scripts");
                CATCH_REQUIRE(rc.get_db() == "that/db");
            }

            {
                // create an .rc file, without scripts
                {
                    std::ofstream rc_file;
                    rc_file.open(as2js_rc.c_str());
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "{\n"
                            << "  'scripts': 'the/script'\n"
                            << "}\n";
                }

                rc.init(true);
                unlink(as2js_rc.c_str());

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                CATCH_REQUIRE(*paths.begin() == "the/script");
                CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
            }

            {
                // create an .rc file, without scripts
                {
                    std::ofstream rc_file;
                    rc_file.open(as2js_rc.c_str());
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "{\n"
                            << "  'scripts': 123\n"
                            << "}\n";
                }

                test_callback::expected_t expected2;
                expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
                expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_RC;
                expected2.f_pos.set_filename(as2js_rc.c_str());
                expected2.f_pos.set_function("unknown-func");
                expected2.f_pos.new_line();
                expected2.f_pos.new_line();
                expected2.f_pos.new_line();
                expected2.f_message = "a resource file is expected to be an object of string elements.";
                tc.f_expected.push_back(expected2);

                CATCH_REQUIRE_THROWS_MATCHES(
                      rc.init(true)
                    , as2js::as2js_exit
                    , Catch::Matchers::ExceptionMessage(
                              "as2js_exception: a resource file is expected to be an object of string elements."));
                tc.got_called();
                unlink(as2js_rc.c_str());

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                CATCH_REQUIRE(*paths.begin() == "as2js/scripts");
                CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
            }

            {
                // create a null .rc file
                {
                    std::ofstream rc_file;
                    rc_file.open(as2js_rc.c_str());
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "null\n";
                }

                rc.init(false);
                unlink(as2js_rc.c_str());

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                CATCH_REQUIRE(*paths.begin() == "as2js/scripts");
                CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
            }

            {
                // create an .rc file, without an object nor null
                {
                    std::ofstream rc_file;
                    rc_file.open(as2js_rc.c_str());
                    CATCH_REQUIRE(rc_file.is_open());
                    rc_file << "// rc file\n"
                            << "['scripts', 123]\n";
                }

                test_callback::expected_t expected2;
                expected2.f_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
                expected2.f_error_code = as2js::err_code_t::AS_ERR_UNEXPECTED_RC;
                expected2.f_pos.set_filename(as2js_rc.c_str());
                expected2.f_pos.set_function("unknown-func");
                expected2.f_pos.new_line();
                expected2.f_message = "a resource file (.rc) must be defined as a JSON object, or set to \"null\".";
                tc.f_expected.push_back(expected2);

                CATCH_REQUIRE_THROWS_MATCHES(
                      rc.init(true)
                    , as2js::as2js_exit
                    , Catch::Matchers::ExceptionMessage(
                              "as2js_exception: a resource file (.rc) must be defined as a JSON object, or set to \"null\"."));
                tc.got_called();
                unlink(as2js_rc.c_str());

                as2js::resources::script_paths_t paths(rc.get_scripts());
                CATCH_REQUIRE(paths.size() == 1);
                CATCH_REQUIRE(*paths.begin() == "as2js/scripts");
                CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
            }
        }

        // delete our temporary .rc file (should already have been deleted)
        unlink(as2js_rc.c_str());

        // if possible get rid of the directories (don't check for errors)
        rmdir(as2js_conf.c_str());
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("resources_empty_home", "[resources][config][file]")
{
    CATCH_START_SECTION("resources_empty_home: NULL value")
    {
        // this test is not going to work if the get_home() function was
        // already called...
        if(g_empty_home_too_late == 1)
        {
            std::cout << "--- test_empty_home() not run, the other rc unit tests are not compatible with this test ---\n";
            CATCH_REQUIRE(g_empty_home_too_late == 1);
            return;
        }

        g_empty_home_too_late = 2;

        // create an .rc file in the user's config directory
        //
        std::string home(getenv("HOME"));

        std::string config(home);
        config += "/.config";
        std::cout << "--- config path \"" << config << "\" ---\n";
        bool del_config(true);
        if(mkdir(config.c_str(), 0700) != 0) // usually this is 0755, but for security we cannot take that risk...
        {
            // if this mkdir() fails, it is because it exists?
            //
            CATCH_REQUIRE(errno == EEXIST);
            del_config = false;
        }

        std::string rc_path(config);
        rc_path += "/as2js";
        CATCH_REQUIRE(mkdir(rc_path.c_str(), 0700) == 0);

        std::string rc_filename(rc_path);
        rc_filename += "/as2js.rc";

        std::ofstream rc_file;
        rc_file.open(rc_filename.c_str());
        CATCH_REQUIRE(rc_file.is_open());
        rc_file << "// rc file\n"
                << "{\n"
                << "  'scripts': 'cannot read this one',\n"
                << "  'db': 'because it is not accessible'\n"
                << "}\n";

        // remove the variable from the environment
        //
        snapdev::transparent_setenv safe("HOME", std::string());

        {
            test_callback tc;

            // although we have an rc file under ~/.config/as2js/as2js.rc the
            // rc class cannot find it because the $HOME variable was just deleted
            as2js::resources rc;
            rc.init(true);

            as2js::resources::script_paths_t paths(rc.get_scripts());
            CATCH_REQUIRE(paths.size() == 1);
            CATCH_REQUIRE(*paths.begin() == "as2js/scripts");
            CATCH_REQUIRE(rc.get_db() == "/tmp/as2js_packages.db");
        }

        unlink(rc_filename.c_str());
        rmdir(rc_path.c_str());
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
