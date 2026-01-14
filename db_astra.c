#include "db_astra.h"
#include <stdio.h>

// untuk melihat error dari ODBC
void show_error(unsigned int handleType, const SQLHANDLE handle)
{
    SQLCHAR sqlState[1024];
    SQLCHAR message[1024];
    if (SQLGetDiagRec(handleType, handle, 1, sqlState, NULL, message, 1024, NULL) == SQL_SUCCESS)
    {
        printf("SQL Error: %s - %s\n", sqlState, message);
    }
}
// koneksi ke database
int astra_connect(AstraDB *db, const char *dsn_name)
{
    if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &db->hEnv) != SQL_SUCCESS)
        return 0;
    if (SQLSetEnvAttr(db->hEnv, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0) != SQL_SUCCESS)
        return 0;
    if (SQLAllocHandle(SQL_HANDLE_DBC, db->hEnv, &db->hDbc) != SQL_SUCCESS)
        return 0;

    char connStr[255];
    // Pastikan Anda sudah membuat DSN di "ODBC Data Sources (32-bit/64-bit)"
    sprintf(connStr, "DSN=%s;", dsn_name);

    SQLCHAR outStr[1024];
    SQLSMALLINT outLen;
    SQLRETURN ret = SQLDriverConnect(db->hDbc, NULL, (SQLCHAR *)connStr, SQL_NTS,
                                     outStr, sizeof(outStr), &outLen, SQL_DRIVER_COMPLETE);

    if (SQL_SUCCEEDED(ret))
    {
        db->isConnected = 1;
        db->hStmt = NULL;
        printf("Berhasil terhubung ke Database!\n");
        return 1;
    }
    else
    {
        show_error(SQL_HANDLE_DBC, db->hDbc);
        db->isConnected = 0;
        return 0;
    }
}

void astra_disconnect(AstraDB *db)
{
    if (db->hStmt)
        SQLFreeHandle(SQL_HANDLE_STMT, db->hStmt);
    if (db->isConnected)
        SQLDisconnect(db->hDbc);
    SQLFreeHandle(SQL_HANDLE_DBC, db->hDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, db->hEnv);
}

// Cek Role: 0 = Gagal, 1 = Admin, 2 = User
int astra_register(AstraDB *db, const char *user, const char *email_addr, const char *pass, const char *role_name)
{
    char query[512];

    sprintf(query, "INSERT INTO users (username, email_address, password_hash, role) VALUES ('%s', '%s', '%s', '%s')",
            user, email_addr, pass, role_name);

    printf("SQL: %s\n", query);

    return astra_execute(db, query);
}

int 
astra_query(AstraDB *db, const char *sql)
{
    SQLRETURN ret;
    if (db->hStmt)
        SQLFreeStmt(db->hStmt, SQL_CLOSE);
    else
        SQLAllocHandle(SQL_HANDLE_STMT, db->hDbc, &db->hStmt);

    ret = SQLExecDirect(db->hStmt, (SQLCHAR *)sql, SQL_NTS);
    if (!SQL_SUCCEEDED(ret))
    {
        show_error(SQL_HANDLE_STMT, db->hStmt); // Tampilkan error query jika gagal
        return 0;
    }
    return 1;
}

int astra_fetch_row(AstraDB *db)
{
    if (SQLFetch(db->hStmt) == SQL_SUCCESS)
    {
        // Ambil data kolom ke-1 (index ODBC mulai dari 1)
        SQLGetData(db->hStmt, 1, SQL_C_CHAR, db->col[0], sizeof(db->col[0]), NULL);
        return 1;
    }
    return 0;
}

int astra_execute(AstraDB *db, const char *sql)
{
    return astra_query(db, sql);
}