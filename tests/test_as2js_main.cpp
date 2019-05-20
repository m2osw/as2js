/* tests/test_as2js_main.cpp

Copyright (c) 2005-2019  Made to Order Software Corp.  All Rights Reserved

https://snapwebsites.org/project/as2js

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and
associated documentation files (the "Software"), to
deal in the Software without restriction, including
without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include    "test_as2js_main.h"
#include    "license.h"

#include    "as2js/as2js.h"

#include    <advgetopt/advgetopt.h>

#include    <cppunit/BriefTestProgressListener.h>
#include    <cppunit/CompilerOutputter.h>
#include    <cppunit/extensions/TestFactoryRegistry.h>
#include    <cppunit/TestRunner.h>
#include    <cppunit/TestResult.h>
#include    <cppunit/TestResultCollector.h>

//#include "time.h"
#include    <unistd.h>

#ifdef HAVE_QT4
#include    <qxcppunit/testrunner.h>
#include    <QApplication>
#endif


namespace as2js_test
{

std::string     g_tmp_dir;
std::string     g_as2js_compiler;
bool            g_gui = false;
bool            g_run_stdout_destructive = false;
bool            g_save_parser_tests = false;

}


// Recursive dumps the given Test heirarchy to cout
namespace
{
void dump(CPPUNIT_NS::Test *test, std::string indent)
{
    if(test)
    {
        std::cout << indent << test->getName() << std::endl;

        // recursive for children
        indent += "  ";
        int max(test->getChildTestCount());
        for(int i = 0; i < max; ++i)
        {
            dump(test->getChildTestAt(i), indent);
        }
    }
}

template<class R>
void add_tests(const advgetopt::getopt& opt, R& runner)
{
    CPPUNIT_NS::Test *root(CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest());
    int max(opt.size("filename"));
    if(max == 0 || opt.is_defined("all"))
    {
        if(max != 0)
        {
            fprintf(stderr, "unittest: named tests on the command line will be ignored since --all was used.\n");
        }
        CPPUNIT_NS::Test *all_tests(root->findTest("All Tests"));
        if(all_tests == nullptr)
        {
            // this should not happen because cppunit throws if they do not find
            // the test you specify to the findTest() function
            std::cerr << "error: no tests were found." << std::endl;
            exit(1);
        }
        runner.addTest(all_tests);
    }
    else
    {
        for(int i = 0; i < max; ++i)
        {
            std::string test_name(opt.get_string("filename", i));
            CPPUNIT_NS::Test *test(root->findTest(test_name));
            if(test == nullptr)
            {
                // this should not happen because cppunit throws if they do not find
                // the test you specify to the findTest() function
                std::cerr << "error: test \"" << test_name << "\" was not found." << std::endl;
                exit(1);
            }
            runner.addTest(test);
        }
    }
}
}

#pragma GCC diagnostic ignored "-Wpedantic"
int unittest_main(int argc, char *argv[])
{
    static const advgetopt::option options[] = {
        {
            'a',
            advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG,
            "all",
            nullptr,
            "run all the tests in the console (default)",
            nullptr
        },
        {
            '\0',
            advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG,
            "destructive",
            nullptr,
            "also run the stdout destructive test (otherwise skip the test so we do not lose stdout)",
            nullptr
        },
        {
            'g',
            advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG,
            "gui",
            nullptr,
#ifdef HAVE_QT4
            "start the GUI version if available",
#else
            "GUI version not available; this option will fail",
#endif
            nullptr
        },
        {
            'h',
            advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG,
            "help",
            nullptr,
            "print out this help screen",
            nullptr
        },
        {
            '\0',
            advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG,
            "license",
            nullptr,
            "prints out the license of the tests",
            nullptr
        },
        {
            '\0',
            advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG,
            "licence",
            nullptr,
            nullptr, // hide this one from the help screen
            nullptr
        },
        {
            'l',
            advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG,
            "list",
            nullptr,
            "list all the available tests",
            nullptr
        },
        {
            'S',
            advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED,
            "seed",
            nullptr,
            "value to seed the randomizer",
            nullptr
        },
        {
            '\0',
            advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG,
            "save-parser-tests",
            nullptr,
            "save the JSON used to test the parser",
            nullptr
        },
        {
            't',
            advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED,
            "tmp",
            nullptr,
            "path to a temporary directory",
            nullptr
        },
        {
            '\0',
            advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_REQUIRED,
            "as2js",
            nullptr,
            "path to the as2js executable",
            nullptr
        },
        {
            'V',
            advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_FLAG,
            "version",
            nullptr,
            "print out the as2js project version these unit tests pertain to",
            nullptr
        },
        {
            '\0',
            advgetopt::GETOPT_FLAG_COMMAND_LINE | advgetopt::GETOPT_FLAG_MULTIPLE | advgetopt::GETOPT_FLAG_DEFAULT_OPTION,
            "filename",
            nullptr,
            nullptr, // hidden argument in --help screen
            nullptr
        },
        {
            '\0',
            advgetopt::GETOPT_FLAG_END,
            nullptr,
            nullptr,
            nullptr,
            nullptr
        }
    };
    static const advgetopt::options_environment options_env =
    {
        .f_project_name = "test_as2js_main",
        .f_options = options,
        .f_options_files_directory = nullptr,
        .f_environment_variable_name = "UNITTEST_OPTIONS",
        .f_configuration_files = nullptr,
        .f_configuration_filename = nullptr,
        .f_configuration_directories = nullptr,
        .f_environment_flags = 0,
        .f_help_header = "Usage: %p [--opt] [test-name]"
                         "with --opt being one or more of the following:",
        .f_help_footer = nullptr,
        .f_version = AS2JS_VERSION,
        .f_license = nullptr,
        .f_copyright = nullptr,
        //.f_build_date = __DATE__,
        //.f_build_time = __TIME__
    };

    advgetopt::getopt opt(options_env, argc, argv);

    if(opt.is_defined("help"))
    {
        std::cerr << opt.usage(advgetopt::GETOPT_FLAG_SHOW_ALL);
        exit(1);
    }

    if(opt.is_defined("version"))
    {
        std::cout << AS2JS_VERSION << std::endl;
        exit(1);
    }

    if(opt.is_defined("license") || opt.is_defined("licence"))
    {
        as2js_tools::license::license();
        exit(1);
    }

    if(opt.is_defined("list"))
    {
        CPPUNIT_NS::Test *all = CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest();
        dump(all, "");
        exit(1);
    }
    as2js_test::g_run_stdout_destructive = opt.is_defined("destructive");

    as2js_test::g_save_parser_tests = opt.is_defined("save-parser-tests");

    // by default we get a different seed each time; that really helps
    // in detecting errors! (I know, I wrote loads of tests before)
    unsigned int seed(static_cast<unsigned int>(time(nullptr)));
    if(opt.is_defined("seed"))
    {
        seed = static_cast<unsigned int>(opt.get_long("seed"));
    }
    srand(seed);
    std::cout << opt.get_program_name() << "[" << getpid() << "]" << ": version " << AS2JS_VERSION << ", seed is " << seed << std::endl;
    std::ofstream seed_file;
    seed_file.open("seed.txt");
    if(seed_file.is_open())
    {
        seed_file << seed << std::endl;
    }

    if(opt.is_defined("tmp"))
    {
        as2js_test::g_tmp_dir = opt.get_string("tmp");
    }
    if(opt.is_defined("as2js"))
    {
        as2js_test::g_as2js_compiler = opt.get_string("as2js");
    }

    if(opt.is_defined("gui"))
    {
#ifdef HAVE_QT4
        as2js_test::g_gui = true;
        QApplication app(argc, argv);
        QxCppUnit::TestRunner runner;
        add_tests(opt, runner);
        runner.run();
#else
        std::cerr << "error: no GUI compiled in this test, you cannot use the --gui option.\n";
        exit(1);
#endif
    }
    else
    {
        // Create the event manager and test controller
        CPPUNIT_NS::TestResult controller;

        // Add a listener that colllects test result
        CPPUNIT_NS::TestResultCollector result;
        controller.addListener(&result);        

        // Add a listener that print dots as test run.
        CPPUNIT_NS::BriefTestProgressListener progress;
        controller.addListener(&progress);      

        CPPUNIT_NS::TestRunner runner;

        add_tests(opt, runner);

        runner.run(controller);

        // Print test in a compiler compatible format.
        CPPUNIT_NS::CompilerOutputter outputter(&result, CPPUNIT_NS::stdCOut());
        outputter.write(); 

        if(result.testFailuresTotal())
        {
            return 1;
        }
    }

    return 0;
}


int main(int argc, char *argv[])
{
    return unittest_main(argc, argv);
}

// vim: ts=4 sw=4 et
