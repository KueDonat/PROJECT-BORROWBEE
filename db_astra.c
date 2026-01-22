#include "db_astra.h"
#include <stdio.h>
#include <string.h> // Dibutuhkan untuk sprintf

// ----------------------------------------------------------------------------
// HELPER: Menampilkan Error ODBC
// ----------------------------------------------------------------------------
void show_error(unsigned int handleType, const SQLHANDLE handle)
{
    SQLCHAR sqlState[1024];
    SQLCHAR message[1024];
    if (SQLGetDiagRec(handleType, handle, 1, sqlState, NULL, message, 1024, NULL) == SQL_SUCCESS)
    {
        printf("SQL Error: %s - %s\n", sqlState, message);
    }
}

// ----------------------------------------------------------------------------
// KONEKSI DATABASE
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
// FUNGSI DASAR QUERY
// ----------------------------------------------------------------------------
int astra_query(AstraDB *db, const char *sql)
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

int astra_execute(AstraDB *db, const char *sql)
{
    return astra_query(db, sql);
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

// ----------------------------------------------------------------------------
// FITUR: REGISTRASI & USERS
// ----------------------------------------------------------------------------
// Update fungsi astra_register agar sesuai kolom database 'nama_lengkap'
int astra_register(AstraDB *db, const char *fullname, const char *user, const char *email_addr, const char *pass, const char *role_name)
{
    char query[1024];

    // PERUBAHAN DISINI:
    // Ganti 'full_name' menjadi 'nama_lengkap' sesuai tabel database kamu
    sprintf(query, "INSERT INTO users (nama_lengkap, username, email_address, password_hash, role, created_at) VALUES ('%s', '%s', '%s', '%s', '%s', GETDATE())",
            fullname, user, email_addr, pass, role_name);

    printf("SQL: %s\n", query); // Debugging: cek query di terminal

    return astra_execute(db, query);
}

// ----------------------------------------------------------------------------
// FITUR BARU: VALIDASI TOKEN / OTP ADMIN
// ----------------------------------------------------------------------------

// 1. Cek Token: Return 1 jika token ada dan belum dipakai, 0 jika invalid.
int astra_check_token(AstraDB *db, const char *token)
{
    SQLHSTMT hStmtTemp;

    // Kita buat statement handle baru (temporary) agar tidak mengganggu query utama
    if (SQLAllocHandle(SQL_HANDLE_STMT, db->hDbc, &hStmtTemp) != SQL_SUCCESS)
    {
        return 0;
    }

    char query[256];
    // Query menghitung jumlah kode yang cocok DAN is_used = 0
    sprintf(query, "SELECT COUNT(*) FROM verification_codes WHERE code = '%s' AND is_used = 0", token);

    if (SQLExecDirect(hStmtTemp, (SQLCHAR *)query, SQL_NTS) != SQL_SUCCESS)
    {
        show_error(SQL_HANDLE_STMT, hStmtTemp);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmtTemp);
        return 0;
    }

    long count = 0;
    if (SQLFetch(hStmtTemp) == SQL_SUCCESS)
    {
        // Ambil hasil COUNT(*)
        SQLGetData(hStmtTemp, 1, SQL_C_LONG, &count, 0, NULL);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmtTemp);

    // Jika count > 0 berarti token valid dan tersedia
    return (count > 0) ? 1 : 0;
}

// 2. Gunakan Token: Update status token menjadi 'sudah dipakai' (hangus)
void astra_use_token(AstraDB *db, const char *token)
{
    char query[256];
    sprintf(query, "UPDATE verification_codes SET is_used = 1 WHERE code = '%s'", token);

    // Kita bisa gunakan fungsi astra_execute yang sudah ada
    if (astra_execute(db, query))
    {
        printf("[DB] Token '%s' berhasil dihanguskan.\n", token);
    }
    else
    {
        printf("[DB] Gagal menghanguskan token.\n");
    }
}