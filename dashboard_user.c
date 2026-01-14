#include "common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ============================================================================
// 1. KONFIGURASI TAMPILAN (Warna & Ukuran)
// ============================================================================
#define COL_SIDEBAR     (Color){230, 240, 255, 255} // Biru Muda (Sidebar)
#define COL_CONTENT     (Color){250, 250, 250, 255} // Putih (Area Tabel)
#define COL_ACCENT      (Color){0, 121, 241, 255}   // Biru Utama (Tombol/Header)
#define COL_BTN_LOGOUT  (Color){220, 50, 50, 255}   // Merah (Logout)
#define MAX_ROWS 50 

// Enum Menu Sidebar
typedef enum { 
    MENU_KATALOG, 
    MENU_STATUS, 
    MENU_RIWAYAT 
} UserMenuState;

// Struct Data Peminjaman (Agar mudah dibaca di tabel)
typedef struct {
    char id[10];
    char nama_barang[50];
    char tgl_pinjam[20];
    char tgl_rencana[20];
    char status[20];
    char denda[20];
    char jumlah[10]; 
} DataPinjamUser;

// ============================================================================
// 2. VARIABEL GLOBAL (Penyimpanan Data Sementara)
// ============================================================================
static UserMenuState currentMenu = MENU_KATALOG; // Menu aktif saat ini
static bool needRefresh = true;                  // Penanda reload data

// Cache Data (Hasil dari Database disimpan di sini)
static DataBarang listBarang[MAX_ROWS];
static DataPinjamUser listPinjam[MAX_ROWS];
static int countBarang = 0;
static int countPinjam = 0;

// Variabel Popup (Formulir Peminjaman)
static bool showPopup = false;
static char selectedID[10] = {0};   // ID Barang yang dipilih
static char selectedName[50] = {0}; // Nama Barang yang dipilih
static int selectedStok = 0;        // Stok barang saat ini

// Input User di Popup
static char inputDurasi[10] = {0}; 
static char inputJumlah[10] = {0}; 
static int activeInput = 0; // 1 = Durasi, 2 = Jumlah

// ID User yang sedang login (Diambil dari main.c)
extern int g_currentUserId;

// ============================================================================
// 3. FUNGSI DATABASE (CRUD)
// ============================================================================

// Fungsi untuk mengambil data terbaru dari database
void RefreshUserData(AstraDB *db) {
    printf("Refreshing Data User...\n");

    // A. Ambil Data Katalog Barang
    countBarang = 0;
    if (astra_query(db, "SELECT barang_id, kode_barang, nama_barang, stok_tersedia FROM Barang")) {
        while (SQLFetch(db->hStmt) == SQL_SUCCESS && countBarang < MAX_ROWS) {
            SQLGetData(db->hStmt, 1, SQL_C_CHAR, listBarang[countBarang].id, 10, NULL);
            SQLGetData(db->hStmt, 2, SQL_C_CHAR, listBarang[countBarang].kode, 20, NULL);
            SQLGetData(db->hStmt, 3, SQL_C_CHAR, listBarang[countBarang].nama, 100, NULL);
            SQLGetData(db->hStmt, 4, SQL_C_CHAR, listBarang[countBarang].stok, 10, NULL);
            countBarang++;
        }
    }

    // B. Ambil Data Peminjaman (Sesuai Menu Status/Riwayat)
    countPinjam = 0;
    char query[1024];

    if (currentMenu == MENU_STATUS) {
        // Ambil data yang masih AKTIF (Pending/Dipinjam)
        sprintf(query, "SELECT t.id, b.nama_barang, t.tgl_pinjam, t.tgl_rencana_kembali, t.status, '0', CAST(t.jumlah AS VARCHAR) "
                       "FROM TransaksiPeminjaman t JOIN Barang b ON t.barang_id = b.barang_id "
                       "WHERE t.user_id = %d AND t.status IN ('Pending', 'Dipinjam', 'Ditolak') ORDER BY t.id DESC", g_currentUserId);
    } else {
        // Ambil data RIWAYAT (Sudah Kembali)
        sprintf(query, "SELECT t.id, b.nama_barang, t.tgl_pinjam, t.tgl_rencana_kembali, t.status, CAST(t.denda AS VARCHAR), CAST(t.jumlah AS VARCHAR) "
                       "FROM TransaksiPeminjaman t JOIN Barang b ON t.barang_id = b.barang_id "
                       "WHERE t.user_id = %d AND t.status = 'Kembali' ORDER BY t.id DESC", g_currentUserId);
    }

    if (astra_query(db, query)) {
        while (SQLFetch(db->hStmt) == SQL_SUCCESS && countPinjam < MAX_ROWS) {
            SQLGetData(db->hStmt, 1, SQL_C_CHAR, listPinjam[countPinjam].id, 10, NULL);
            SQLGetData(db->hStmt, 2, SQL_C_CHAR, listPinjam[countPinjam].nama_barang, 50, NULL);
            SQLGetData(db->hStmt, 3, SQL_C_CHAR, listPinjam[countPinjam].tgl_pinjam, 20, NULL);
            SQLGetData(db->hStmt, 4, SQL_C_CHAR, listPinjam[countPinjam].tgl_rencana, 20, NULL);
            SQLGetData(db->hStmt, 5, SQL_C_CHAR, listPinjam[countPinjam].status, 20, NULL);
            SQLGetData(db->hStmt, 6, SQL_C_CHAR, listPinjam[countPinjam].denda, 20, NULL);
            SQLGetData(db->hStmt, 7, SQL_C_CHAR, listPinjam[countPinjam].jumlah, 10, NULL);
            countPinjam++;
        }
    }
    needRefresh = false;
}

// Fungsi Proses Pengajuan Peminjaman
void ProsesAjukan(AstraDB *db) {
    if (strlen(inputDurasi) == 0 || strlen(inputJumlah) == 0) return;
    
    int durasi = atoi(inputDurasi);
    int jumlah = atoi(inputJumlah);

    // Validasi Sederhana
    if (durasi <= 0 || jumlah <= 0) return;
    if (jumlah > selectedStok) {
        printf("Stok barang tidak cukup!\n"); 
        return;
    }

    // 1. Simpan Transaksi Baru
    char qInsert[512];
    sprintf(qInsert, "INSERT INTO TransaksiPeminjaman (user_id, barang_id, jumlah, tgl_pinjam, tgl_rencana_kembali, status, denda) "
                     "VALUES (%d, %s, %d, GETDATE(), DATEADD(day, %d, GETDATE()), 'Pending', 0)", 
                     g_currentUserId, selectedID, jumlah, durasi);
    astra_execute(db, qInsert);

    // 2. Kurangi Stok Barang
    char qUpdate[512];
    sprintf(qUpdate, "UPDATE Barang SET stok_tersedia = stok_tersedia - %d WHERE barang_id = %s", jumlah, selectedID);
    astra_execute(db, qUpdate);

    // Reset & Refresh
    showPopup = false;
    needRefresh = true;
    currentMenu = MENU_STATUS; // Pindah ke menu status agar user lihat hasilnya
}

// ============================================================================
// 4. HELPER UI (Fungsi Pembantu Menggambar)
// ============================================================================

// Menggambar Tombol
static bool DrawBtn(float x, float y, float w, const char* text, Color color) {
    Rectangle rect = {x, y, w, 30};
    bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
    DrawRectangleRec(rect, hover ? Fade(color, 0.8f) : color);
    DrawText(text, x + 10, y + 5, 10, WHITE);
    return (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
}

// Input Angka (Keyboard)
void InputAngka(char *buffer, bool isActive) {
    if (!isActive) return;
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= '0' && key <= '9' && strlen(buffer) < 5) {
            int len = strlen(buffer); buffer[len] = (char)key; buffer[len+1] = 0;
        }
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && strlen(buffer) > 0) buffer[strlen(buffer)-1] = 0;
}

// ============================================================================
// 5. TAMPILAN UTAMA (Fungsi Draw)
// ============================================================================

void DrawTabelKatalog(float x, float y, float w) {
    // Header
    DrawText("KODE", x+10, y, 20, BLACK);
    DrawText("NAMA BARANG", x+100, y, 20, BLACK);
    DrawText("STOK", x+400, y, 20, BLACK);
    DrawText("AKSI", x+w-100, y, 20, BLACK);
    DrawLine(x, y+25, x+w, y+25, BLACK);

    for (int i=0; i<countBarang; i++) {
        float ry = y + 40 + (i * 40);
        DrawText(listBarang[i].kode, x+10, ry, 20, DARKGRAY);
        DrawText(listBarang[i].nama, x+100, ry, 20, DARKGRAY);
        
        int stok = atoi(listBarang[i].stok);
        DrawText(listBarang[i].stok, x+400, ry, 20, (stok>0)? DARKGREEN : RED);

        if (stok > 0) {
            if (DrawBtn(x+w-100, ry, 80, "PINJAM", COL_ACCENT)) {
                // Buka Popup Form
                showPopup = true;
                strcpy(selectedID, listBarang[i].id);
                strcpy(selectedName, listBarang[i].nama);
                selectedStok = stok;
                memset(inputDurasi, 0, 10); memset(inputJumlah, 0, 10);
                activeInput = 1; // Fokus ke Durasi
            }
        } else {
            DrawText("Habis", x+w-100, ry, 20, RED);
        }
    }
}

void DrawTabelStatus(float x, float y, float w) {
    DrawText("BARANG", x+10, y, 20, BLACK);
    DrawText("JML & TGL", x+200, y, 20, BLACK);
    DrawText("BATAS", x+450, y, 20, BLACK);
    DrawText("STATUS", x+650, y, 20, BLACK);
    DrawLine(x, y+25, x+w, y+25, BLACK);

    for (int i=0; i<countPinjam; i++) {
        float ry = y + 40 + (i * 40);
        DrawText(listPinjam[i].nama_barang, x+10, ry, 20, DARKGRAY);
        DrawText(TextFormat("(%sx) %s", listPinjam[i].jumlah, listPinjam[i].tgl_pinjam), x+200, ry, 20, DARKGRAY);
        DrawText(listPinjam[i].tgl_rencana, x+450, ry, 20, DARKGRAY);
        
        Color stCol = BLACK;
        if (strcmp(listPinjam[i].status, "Pending") == 0) stCol = ORANGE;
        else if (strcmp(listPinjam[i].status, "Dipinjam") == 0) stCol = BLUE;
        else if (strcmp(listPinjam[i].status, "Ditolak") == 0) stCol = RED;
        else if (strcmp(listPinjam[i].status, "Kembali") == 0) stCol = GREEN;

        DrawText(listPinjam[i].status, x+650, ry, 20, stCol);
    }
}

// --- FUNGSI DRAW DASHBOARD USER ---
AppState DrawDashboardUser(AstraDB *db, Texture2D logo) {
    if (needRefresh) RefreshUserData(db);
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // 1. Sidebar (Kiri)
    DrawRectangle(0, 0, 250, sh, COL_SIDEBAR);
    if (logo.id > 0) DrawTexturePro(logo, (Rectangle){0,0,logo.width,logo.height}, (Rectangle){25, 30, 200, logo.height*(200.0f/logo.width)}, (Vector2){0,0}, 0.0f, WHITE);
    else DrawText("BorrowBee", 40, 40, 30, COL_ACCENT);

    // Menu Sidebar
    const char *labels[] = {"KATALOG BARANG", "STATUS PINJAMAN", "RIWAYAT KEMBALI"};
    UserMenuState states[] = {MENU_KATALOG, MENU_STATUS, MENU_RIWAYAT};

    for (int i=0; i<3; i++) {
        Rectangle btn = {20, 150+(i*60), 210, 40};
        bool active = (currentMenu == states[i]);

        if (active) DrawRectangleRec(btn, WHITE);
        if (CheckCollisionPointRec(GetMousePosition(), btn)) {
            DrawRectangleLinesEx(btn, 2, COL_ACCENT);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { currentMenu = states[i]; needRefresh = true; }
        }
        DrawText(labels[i], (int)btn.x+20, (int)btn.y+10, 18, (active ? COL_ACCENT : BLACK));
    }

    // Tombol Logout (Di Bawah)
    Rectangle btnLogout = {20, sh - 80, 210, 40};
    bool hoverLogout = CheckCollisionPointRec(GetMousePosition(), btnLogout);
    
    if (hoverLogout) {
        DrawRectangleRec(btnLogout, WHITE);
        DrawRectangleLinesEx(btnLogout, 2, COL_BTN_LOGOUT);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { g_currentUserId = 0; return STATE_MENU; }
    }
    DrawText("KELUAR", 40, sh-70, 18, hoverLogout ? COL_BTN_LOGOUT : RED);

    // 2. Konten Utama (Kanan)
    float cx = 280, cy = 100, cw = sw - 320;
    DrawRectangleRec((Rectangle){cx, cy, cw, sh-150}, COL_CONTENT);
    
    if (currentMenu == MENU_KATALOG) {
        DrawText("KATALOG BARANG", cx, 40, 30, BLACK);
        DrawTabelKatalog(cx, cy, cw);
    } else {
        DrawText(currentMenu == MENU_STATUS ? "STATUS PINJAMAN" : "RIWAYAT KEMBALI", cx, 40, 30, BLACK);
        DrawTabelStatus(cx, cy, cw);
    }

    // 3. Popup Form Peminjaman
    if (showPopup) {
        DrawRectangle(0, 0, sw, sh, (Color){0,0,0,150}); // Layar Gelap
        float px = (sw-400)/2, py = (sh-320)/2;
        
        DrawRectangle(px, py, 400, 320, RAYWHITE);
        DrawText("AJUKAN PEMINJAMAN", px+20, py+20, 20, COL_ACCENT);
        DrawText(TextFormat("Barang: %s", selectedName), px+20, py+60, 20, BLACK);
        DrawText(TextFormat("(Stok: %d)", selectedStok), px+20, py+85, 16, DARKGRAY);

        // Input 1: Durasi
        DrawText("Durasi (Hari):", px+20, py+120, 20, DARKGRAY);
        Rectangle r1 = {px+200, py+120, 100, 30};
        DrawRectangleRec(r1, activeInput==1?LIGHTGRAY:WHITE); DrawRectangleLinesEx(r1, 1, COL_ACCENT);
        DrawText(inputDurasi, r1.x+5, r1.y+5, 20, BLACK);
        if (CheckCollisionPointRec(GetMousePosition(), r1) && IsMouseButtonPressed(0)) activeInput = 1;

        // Input 2: Jumlah
        DrawText("Jumlah Barang:", px+20, py+170, 20, DARKGRAY);
        Rectangle r2 = {px+200, py+170, 100, 30};
        DrawRectangleRec(r2, activeInput==2?LIGHTGRAY:WHITE); DrawRectangleLinesEx(r2, 1, COL_ACCENT);
        DrawText(inputJumlah, r2.x+5, r2.y+5, 20, BLACK);
        if (CheckCollisionPointRec(GetMousePosition(), r2) && IsMouseButtonPressed(0)) activeInput = 2;

        // Handle Input
        if (activeInput == 1) InputAngka(inputDurasi, true);
        if (activeInput == 2) InputAngka(inputJumlah, true);

        // Tombol
        if (DrawBtn(px+20, py+250, 150, "AJUKAN", COL_ACCENT)) ProsesAjukan(db);
        if (DrawBtn(px+190, py+250, 150, "BATAL", RED)) showPopup = false;
    }

    return STATE_DASHBOARD_USER;
}