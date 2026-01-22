#ifndef DB_ASTRA_H
#define DB_ASTRA_H

// Mencegah Windows mendefinisikan min() dan max() yang mengganggu logika C++ atau math
#define NOMINMAX
// Mencegah Windows mendefinisikan banyak fungsi grafis GDI (opsional, tapi membantu)
#define NOGDI

#include <stdio.h>

// ---------------------------------------------------------
// PERBAIKAN KONFLIK NAMA (WINDOWS API vs RAYLIB)
// ---------------------------------------------------------
// Kita "mengganti nama" fungsi Windows sementara agar tidak
// menimpa fungsi Raylib yang bernama sama.

#define CloseWindow WinCloseWindow
#define ShowCursor WinShowCursor
#define DrawText WinDrawText
#define DrawTextEx WinDrawTextEx // <--- BARU: Mengatasi error DrawTextExA
#define LoadImage WinLoadImage   // <--- BARU: Mengatasi error LoadImageA
#define PlaySound WinPlaySound   // <--- BARU: Mengatasi error PlaySoundA
#define Rectangle WinRectangle

// Sertakan header Windows setelah rename di atas
#include <windows.h>
#include <sql.h>
#include <sqlext.h>

// Kembalikan nama fungsi agar Raylib bisa menggunakannya
#undef CloseWindow
#undef ShowCursor
#undef DrawText
#undef DrawTextEx // <--- BARU
#undef LoadImage  // <--- BARU
#undef PlaySound  // <--- BARU
#undef Rectangle

// ---------------------------------------------------------

// Struktur Database
typedef struct
{
    SQLHENV hEnv;
    SQLHDBC hDbc;
    SQLHSTMT hStmt;
    int isConnected;
    char col[10][255];
} AstraDB;

// Variabel Global (Hanya deklarasi extern)
extern AstraDB g_db;

// Fungsi Koneksi
int astra_connect(AstraDB *db, const char *dsn_name);
void astra_disconnect(AstraDB *db);

// Fungsi Logika Bisnis
// Tambahkan parameter const char *fullname
int astra_register(AstraDB *db, const char *fullname, const char *user, const char *email_addr, const char *pass, const char *role_name);

// Fungsi Helper SQL
int astra_query(AstraDB *db, const char *sql);
int astra_fetch_row(AstraDB *db);
int astra_execute(AstraDB *db, const char *sql);
void show_error(unsigned int handleType, const SQLHANDLE handle);
// Cek apakah token ada dan belum terpakai. Return 1 jika valid, 0 jika tidak.
int astra_check_token(AstraDB *conn, const char *token);

// Tandai token sebagai 'sudah dipakai' agar tidak bisa dipakai 2x
void astra_use_token(AstraDB *conn, const char *token);

#endif