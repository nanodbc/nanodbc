#include "catch/catch_amalgamated.hpp"

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
        Catch::Session session;

        // Catch2 v3 runs test cases in a random order by default, where v2 ran them in
        // declaration order. Several of these tests read state an earlier test created, so
        // a shuffled run fails intermittently. Keep the order v2 gave until those
        // dependencies are gone; --order on the command line still overrides this.
        session.configData().runOrder = Catch::TestRunOrder::Declared;

        // Add the connection string option to Catch's own parser, rather than parsing
        // argv separately and blanking out what Catch would not recognise.
        using namespace Catch::Clara;
        session.cli(
            session.cli() | Opt(cfg.connection_string_, "connection")["-z"]["--connection-string"](
                                "connection string to test database; if not specified, "
                                "an attempt will be made to read it from environment variables: "
                                "NANODBC_TEST_CONNSTR or NANODBC_TEST_CONNSTR_<DB>"));

        // Path to data folder with data files used in some tests
#ifdef NANODBC_TEST_DATA
        if (cfg.data_path_.empty())
            cfg.data_path_ = std::string(NANODBC_TEST_DATA);
#endif

        if (session.applyCommandLine(argc, argv) != 0)
            return EXIT_FAILURE;

        // Run tests
        if (session.run() == 0)
            return EXIT_SUCCESS;
    }
    catch (std::exception const& e)
    {
        std::cerr << "\nError(s):\n" << e.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "\nError(s): uncaught exception\n";
    }
    return EXIT_FAILURE;
}
