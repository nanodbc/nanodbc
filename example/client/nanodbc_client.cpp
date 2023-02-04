#include <nanodbc/nanodbc.h>

#include <cstdlib>
#include <exception>
#include <iostream>

int main(int argc, char* argv[])
try
{
    nanodbc::connection conn;
    try
    {
        std::cout << conn.driver_name() << std::endl;
    }
    catch (nanodbc::database_error const& e)
    {
        std::cout << "Connection not open - OK" << std::endl;
    }
    return EXIT_SUCCESS;
}
catch (std::exception const& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
