#include "example_unicode_utils.h"
#include <nanodbc/nanodbc.h>

#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;
using namespace nanodbc;

int main()
{
    try
    {
        connection conn(NANODBC_TEXT("NorthWind"));
        result row = execute(
            conn,
            NANODBC_TEXT("SELECT CustomerID, ContactName, Phone"
                         "   FROM CUSTOMERS"
                         "   ORDER BY 2, 1, 3"));

        for (int i = 1; row.next(); ++i)
        {
            cout << i << " :" << convert(row.get<nanodbc::string>(0)) << " "
                 << convert(row.get<nanodbc::string>(1)) << " "
                 << convert(row.get<nanodbc::string>(2)) << " " << endl;
        }
        return EXIT_SUCCESS;
    }
    catch (runtime_error const& e)
    {
        cerr << e.what() << endl;
    }
    return EXIT_FAILURE;
}
