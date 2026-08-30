#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
int main(int argc, char** argv)
{
    SQLHENV env; SQLHDBC dbc; SQLHSTMT s;
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
    SQLCHAR out[1024]; SQLSMALLINT ol;
    SQLDriverConnect(dbc, NULL, (SQLCHAR*)argv[1], SQL_NTS, out, sizeof(out), &ol, SQL_DRIVER_NOPROMPT);
    printf("sizeof(SQLLEN)=%zu\n", sizeof(SQLLEN));

    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &s);
    SQLExecDirect(s, (SQLCHAR*)"drop table nulltest", SQL_NTS);
    SQLExecDirect(s, (SQLCHAR*)"create table nulltest (i integer)", SQL_NTS);
    SQLExecDirect(s, (SQLCHAR*)"insert into nulltest values (7)", SQL_NTS);
    SQLExecDirect(s, (SQLCHAR*)"insert into nulltest values (null)", SQL_NTS);
    SQLFreeHandle(SQL_HANDLE_STMT, s);
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &s);
    SQLINTEGER iv; SQLLEN ind_i;
    SQLExecDirect(s, (SQLCHAR*)"select i from nulltest order by i", SQL_NTS);
    SQLBindCol(s, 1, SQL_C_SLONG, &iv, 0, &ind_i);
    int row = 0;
    while (1)
    {
        // Poison the whole indicator so we can see exactly which bytes the driver writes.
        memset(&ind_i, 0xAB, sizeof(ind_i));
        if (SQLFetch(s) != SQL_SUCCESS) break;
        unsigned char* b = (unsigned char*)&ind_i;
        printf("row %d bytes:", ++row);
        for (size_t k = 0; k < sizeof(ind_i); ++k) printf(" %02x", b[k]);
        int32_t low; memcpy(&low, &ind_i, sizeof(low));
        printf("   as SQLLEN=%ld   low 32 bits=%d (%s)\n", (long)ind_i, low,
               low == -1 ? "SQL_NULL_DATA" : "length");
    }
    return 0;
}
