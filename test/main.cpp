#define CATCH_CONFIG_RUNNER
#include "catch/catch.hpp"

#include <exception>
#include <iostream>
#include <string>

// clang-format off
#include "base_test_fixture.h" // Must be included last!
// clang-format on

nanodbc::test::Config cfg;

int main(int argc, char* argv[])
{
    try
    {
        // Specify custom command line options
        auto cli
            = Catch::clara::Help(cfg.show_help_)
            | Catch::clara::Opt(cfg.connection_string_, "connection")
                ["-z"]["--connection-string"]
                ("connection string to test database; if not specified, "
                "an attempt will be made to read it from environment variables: "
                 "NANODBC_TEST_CONNSTR or NANODBC_TEST_CONNSTR_<DB>")
            | Catch::clara::Arg(cfg.test_, "test")
                ("test name|pattern|tags to run");
        auto parse_result = cli.parse(Catch::clara::Args(argc, argv));
        if (!parse_result)
        {
            Catch::cerr()
                << Catch::Colour(Catch::Colour::Red)
                << "\nError(s) in input:\n"
                << Catch::Column(parse_result.errorMessage()).indent(2)
                << "\n\n";
            Catch::cerr() << "Run with -? for usage\n" << std::endl;
            return EXIT_FAILURE;
        }

        // Disable custom options to avoid Catch warnings or failures
        for (int i = 1; i < argc; ++i)
        {
            if (strcmp(argv[i], "-z") == 0 || strcmp(argv[i], "--connection-string") == 0)
            {
                *argv[i++] = 0;
                *argv[i] = 0;
            }
        }

        Catch::Session session;
        if (cfg.show_help_)
        {
            session.showHelp();

            Catch::cerr()
                << Catch::Colour(Catch::Colour::Yellow)
                << "nanodbc\n"
                << cli << std::endl;

            return EXIT_FAILURE;
        }

        // Path to data folder with data files used in some tests
#ifdef NANODBC_TEST_DATA
        if (cfg.data_path_.empty())
            cfg.data_path_ = std::string(NANODBC_TEST_DATA);
#endif

        if (session.applyCommandLine(argc, argv) != 0)
            return EXIT_FAILURE;

        // Run tests
        return session.run(argc, argv);
    }
    catch (std::exception const& e)
    {
        Catch::cerr()
            << Catch::Colour(Catch::Colour::Red)
            << "\nError(s):\n"
            << e.what() << '\n';
    }
    catch (...)
    {
        Catch::cerr()
            << Catch::Colour(Catch::Colour::Red)
            << "\nError(s): uncaught exception\n";
    }
    return EXIT_FAILURE;
}
