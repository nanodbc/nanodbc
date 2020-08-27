#include <iostream>
#include <string>
#include "nanodbc.h"

int main() {
    nanodbc::connection conn("NorthWind");
    nanodbc::result row = execute(conn,
        "SELECT CustomerID, ContactName, Phone"
        "   FROM CUSTOMERS"
        "   ORDER BY 2, 1, 3");
    for(int i = 1; row.next(); ++i) {
        std::cout << i << ": "
                  << row.get<std::string>(0) << " "
                  << row.get<std::string>(1) << " "
                  << row.get<std::string>(2) << std::endl;
    }
}
