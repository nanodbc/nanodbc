#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include "external/clara.h"
#include <string>
#include "base_test_fixture.h"

TestConfig cfg;

int main(int argc, char* argv[])
{
    try
    {
        // Specify custom command line options
        auto args = Clara::argsToVector(argc, argv);
        Clara::CommandLine<TestConfig> cli;
        cli["-c"]["--connection-string"]
            .describe("connection string to test database; if not specified, "
                "an attempt will be made to read it from environment variables: "
                "NANODBC_TEST_CONNSTR or NANODBC_TEST_CONNSTR_<DB>")
            .bind(&TestConfig::connection_string_, "string");
        cli.parseInto(args, cfg);

        // Disable custom options to avoid Catch warnings or failures
        for (int i = 1; i < argc; ++i)
        {
            if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--connection-string") == 0)
            {
                *argv[i++] = 0;
                *argv[i] = 0;
            }
        }

        Catch::Session session;
        int result = session.applyCommandLine(argc, argv);
        if (result != 0)
            return EXIT_FAILURE;

        if (session.config().showHelp())
        {
            Catch::cerr() << "ERROR: nanodbc test expected connection string\n\n";
            cli.usage(Catch::cout(), argv[0]);
            session.showHelp(argv[0]);
            return EXIT_FAILURE;
        }

        // Run tests
        result = session.run(argc, argv);
        return result;
    }
    catch (std::exception const& e)
    {
        Catch::cerr() << "ERROR: uncaught exception:\n";
        Catch::cerr() << e.what() << '\n';
    }
}