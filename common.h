#ifndef COMMON_H
#define COMMON_H

#include <raylib.h>
#include "db_astra.h"
extern int g_currentUserId;
extern char g_currentUserRole[20];
// Enum Status Aplikasi
typedef enum
{
    STATE_MENU,
    STATE_REGISTER,
    STATE_LOGIN,
    STATE_DASHBOARD_ADMIN,
    STATE_DASHBOARD_USER,
    STATE_EXIT
} AppState;

// Enum Menu Admin (Tambahkan PEMINJAMAN)
typedef enum
{
    MENU_BARANG,
    MENU_VENDOR,
    MENU_USERS,
    MENU_RUANGAN,
    MENU_PEMINJAMAN,
    MENU_TOKEN
} AdminMenuState;

// --- STRUCT DATA ---
// ... (Struct Barang, Vendor, Ruangan, User TETAP SAMA seperti sebelumnya) ...

typedef struct
{
    char id[10];
    char kode[20];
    char nama[100];
    char stok[10];
} DataBarang;

typedef struct
{
    char id[10];
    char nama[100];
    char alamat[200];
    char no_telp[20];
} DataVendor;

typedef struct
{
    char id[10];
    char nama[100];
    char lokasi[100];
} DataRuangan;

typedef struct
{
    char id[10];
    char username[50];
    char role[20];
    char password[100];
} DataUser;

// --- STRUCT BARU UNTUK PEMINJAMAN ---
typedef struct
{
    char id[10];
    char peminjam[50]; // Dari join users
    char barang[50];   // Dari join barang
    char tgl_pinjam[20];
    char tgl_rencana[20];
    char status[20];
    int hari_terlambat; // Hasil kalkulasi SQL
} DataPeminjaman;

// Deklarasi Fungsi
AppState DrawMenuScreen(Texture2D logo);
AppState DrawLoginScreen(Texture2D logo);
AppState DrawRegisterScreen(Texture2D logo);
void InitDashboardAdmin(AstraDB *db);
AppState DrawDashboardAdmin(AstraDB *db, Texture2D Logo);
AppState DrawDashboardUser(AstraDB *db, Texture2D Logo);

#endif