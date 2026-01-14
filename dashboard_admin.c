// --- START OF FILE dashboard_admin.c ---

#include "common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 

#define MAX_ROWS 50

// --- 1. KONFIGURASI TAMPILAN ---
#define COL_SIDEBAR     (Color){245, 245, 220, 255} 
#define COL_CONTENT     (Color){250, 250, 250, 255} 
#define COL_ACCENT      (Color){255, 165, 0, 255}   
#define COL_BTN_DELETE  (Color){220, 50, 50, 255}
#define COL_BTN_LOGOUT  (Color){220, 50, 50, 255}   
#define COL_TEXT        BLACK 

// --- 2. STATE GLOBAL ---
static AdminMenuState currentMenu = MENU_BARANG;
static bool needRefresh = true;

static DataBarang listBarang[MAX_ROWS];
static DataVendor listVendor[MAX_ROWS];
static DataRuangan listRuangan[MAX_ROWS];
static DataUser listUser[MAX_ROWS];
static DataPeminjaman listPinjam[MAX_ROWS];

static int countBarang = 0, countVendor = 0, countRuangan = 0, countUser = 0, countPinjam = 0;

static bool showPopup = false;
static bool showDendaPopup = false;
static bool showDetailPopup = false;
static int detailIndex = -1;

static int activeInput = 0;
static char in1[100] = {0};
static char in2[100] = {0};
static char in3[100] = {0};

static char currentID[10] = {0};
static char currentTransID[10] = {0};

// --- 3. FUNGSI LOAD DATA ---

void LoadBarang(AstraDB *db) {
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
}

void LoadVendor(AstraDB *db) {
    countVendor = 0;
    if (astra_query(db, "SELECT vendor_id, nama_vendor, alamat, no_telp FROM Vendor")) {
        while (SQLFetch(db->hStmt) == SQL_SUCCESS && countVendor < MAX_ROWS) {
            SQLGetData(db->hStmt, 1, SQL_C_CHAR, listVendor[countVendor].id, 10, NULL);
            SQLGetData(db->hStmt, 2, SQL_C_CHAR, listVendor[countVendor].nama, 100, NULL);
            SQLGetData(db->hStmt, 3, SQL_C_CHAR, listVendor[countVendor].alamat, 200, NULL);
            SQLGetData(db->hStmt, 4, SQL_C_CHAR, listVendor[countVendor].no_telp, 20, NULL);
            countVendor++;
        }
    }
}

void LoadRuangan(AstraDB *db) {
    countRuangan = 0;
    if (astra_query(db, "SELECT ruangan_id, nama_ruangan, lokasi FROM Ruangan")) {
        while (SQLFetch(db->hStmt) == SQL_SUCCESS && countRuangan < MAX_ROWS) {
            SQLGetData(db->hStmt, 1, SQL_C_CHAR, listRuangan[countRuangan].id, 10, NULL);
            SQLGetData(db->hStmt, 2, SQL_C_CHAR, listRuangan[countRuangan].nama, 100, NULL);
            SQLGetData(db->hStmt, 3, SQL_C_CHAR, listRuangan[countRuangan].lokasi, 100, NULL);
            countRuangan++;
        }
    }
}

void LoadUser(AstraDB *db) {
    countUser = 0;
    // Tambahkan password_hash ke dalam query SELECT
    if (astra_query(db, "SELECT user_id, username, role, password_hash FROM users")) {
        while (SQLFetch(db->hStmt) == SQL_SUCCESS && countUser < MAX_ROWS) {
            SQLGetData(db->hStmt, 1, SQL_C_CHAR, listUser[countUser].id, 10, NULL);
            SQLGetData(db->hStmt, 2, SQL_C_CHAR, listUser[countUser].username, 50, NULL);
            SQLGetData(db->hStmt, 3, SQL_C_CHAR, listUser[countUser].role, 20, NULL);
            // Ambil data password (pastikan struct DataUser di common.h punya field 'password')
            SQLGetData(db->hStmt, 4, SQL_C_CHAR, listUser[countUser].password, 100, NULL); 
            countUser++;
        }
    }
}

void LoadPeminjaman(AstraDB *db) {
    countPinjam = 0;
    char *q = "SELECT t.id, u.username, b.nama_barang, t.tgl_pinjam, t.tgl_rencana_kembali, t.status, DATEDIFF(day, t.tgl_rencana_kembali, GETDATE()) FROM TransaksiPeminjaman t JOIN users u ON t.user_id = u.user_id JOIN Barang b ON t.barang_id = b.barang_id WHERE t.status IN ('Pending', 'Dipinjam') ORDER BY t.id DESC";
    if (astra_query(db, q)) {
        while (SQLFetch(db->hStmt) == SQL_SUCCESS && countPinjam < MAX_ROWS) {
            char delayStr[10];
            SQLGetData(db->hStmt, 1, SQL_C_CHAR, listPinjam[countPinjam].id, 10, NULL);
            SQLGetData(db->hStmt, 2, SQL_C_CHAR, listPinjam[countPinjam].peminjam, 50, NULL);
            SQLGetData(db->hStmt, 3, SQL_C_CHAR, listPinjam[countPinjam].barang, 50, NULL);
            SQLGetData(db->hStmt, 4, SQL_C_CHAR, listPinjam[countPinjam].tgl_pinjam, 20, NULL);
            SQLGetData(db->hStmt, 5, SQL_C_CHAR, listPinjam[countPinjam].tgl_rencana, 20, NULL);
            SQLGetData(db->hStmt, 6, SQL_C_CHAR, listPinjam[countPinjam].status, 20, NULL);
            SQLGetData(db->hStmt, 7, SQL_C_CHAR, delayStr, 10, NULL);
            listPinjam[countPinjam].hari_terlambat = atoi(delayStr);
            if(listPinjam[countPinjam].hari_terlambat < 0) listPinjam[countPinjam].hari_terlambat = 0;
            countPinjam++;
        }
    }
}

// --- 4. HELPER UI ---
static bool DrawBtn(float x, float y, float w, const char* text, Color color) {
    Rectangle rect = {x, y, w, 25}; 
    bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
    DrawRectangleRec(rect, hover ? Fade(color, 0.8f) : color);
    DrawText(text, (int)x + (int)(w/2 - MeasureText(text, 10)/2), (int)y + 7, 10, WHITE); 
    return (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
}

static void DrawInput(float x, float y, const char* label, char* buffer, int id) {
    DrawText(label, (int)x, (int)y, 20, DARKGRAY);
    Rectangle rect = {x + 160, y, 280, 30};
    DrawRectangleRec(rect, activeInput == id ? LIGHTGRAY : WHITE);
    DrawRectangleLinesEx(rect, 1, activeInput == id ? COL_ACCENT : BLACK);
    DrawText(buffer, (int)rect.x + 5, (int)rect.y + 5, 20, BLACK);
    if (CheckCollisionPointRec(GetMousePosition(), rect) && IsMouseButtonPressed(0)) activeInput = id;
}

static void HandleInput(char *buffer, int maxLen, bool isActive) {
    if (!isActive) return;
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 125 && strlen(buffer) < (size_t)maxLen) {
            int l = (int)strlen(buffer); buffer[l] = (char)key; buffer[l+1] = 0;
        }
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && strlen(buffer) > 0) buffer[strlen(buffer)-1] = 0;
}

// Header untuk menu umum (barang, vendor, user, ruangan)
static void DrawHeader(float x, float y, float w, const char* c1, const char* c2, const char* c3, const char* c4) {
    DrawRectangle(x, y - 18, w, 45, Fade(LIGHTGRAY, 0.3f));
    
    DrawText(c1, (int)x+10, (int)y, 20, COL_TEXT);
    DrawText(c2, (int)x+100, (int)y, 20, COL_TEXT);
    DrawText(c3, (int)x+300, (int)y, 20, COL_TEXT);
    if(c4) DrawText(c4, (int)x+550, (int)y, 20, COL_TEXT);
    DrawText("AKSI", (int)x+(int)w-150, (int)y, 20, COL_TEXT);
    DrawLine((int)x, (int)y+25, (int)x+(int)w, (int)y+25, COL_TEXT);
}

// Header untuk menu peminjaman
static void DrawHPeminjam(float x, float y, float w, const char* c1, const char* c2, const char* c3, const char* c4) {
    DrawRectangle(x, y - 18, w, 45, Fade(LIGHTGRAY, 0.3f));
    
    DrawText(c1, (int)x+10, (int)y, 20, COL_TEXT);
    DrawText(c2, (int)x+150, (int)y, 20, COL_TEXT);
    DrawText(c3, (int)x+390, (int)y, 20, COL_TEXT);
    if(c4) DrawText(c4, (int)x+630, (int)y, 20, COL_TEXT);
    DrawText("AKSI", (int)x+(int)w-150, (int)y, 20, COL_TEXT);
    DrawLine((int)x, (int)y+25, (int)x+(int)w, (int)y+25, COL_TEXT);
}

// --- 5. LOGIKA DATABASE ---

void SaveBarang(AstraDB *db) {
    char q[512];
    if (strlen(currentID) > 0) snprintf(q, sizeof(q), "UPDATE Barang SET kode_barang='%s', nama_barang='%s', stok_tersedia=%s WHERE barang_id=%s", in1, in2, in3, currentID);
    else sprintf(q, "INSERT INTO Barang (kode_barang, nama_barang, stok_total, stok_tersedia) VALUES ('%s', '%s', %s, %s)", in1, in2, in3, in3);
    astra_execute(db, q); needRefresh = true; showPopup = false;
}

void SaveVendor(AstraDB *db) {
    char q[512];
    if (strlen(currentID) > 0) snprintf(q, sizeof(q), "UPDATE Vendor SET nama_vendor='%s', alamat='%s', no_telp='%s' WHERE vendor_id=%s", in1, in2, in3, currentID);
    else sprintf(q, "INSERT INTO Vendor (nama_vendor, alamat, no_telp) VALUES ('%s', '%s', '%s')", in1, in2, in3);
    astra_execute(db, q); needRefresh = true; showPopup = false;
}

void SaveRuangan(AstraDB *db) {
    char q[512]; if(strlen(currentID)>0) snprintf(q,sizeof(q),"UPDATE Ruangan SET nama_ruangan='%s', lokasi='%s' WHERE ruangan_id=%s",in1,in2,currentID); else sprintf(q,"INSERT INTO Ruangan (nama_ruangan, lokasi) VALUES ('%s', '%s')",in1,in2); astra_execute(db,q); needRefresh=true; showPopup=false;
}

void SaveUser(AstraDB *db) {
    char q[512]; if(strlen(currentID)>0) snprintf(q,sizeof(q),"UPDATE users SET username='%s', password_hash='%s', role='%s' WHERE user_id=%s",in1,in2,in3,currentID); else sprintf(q,"INSERT INTO users (username, password_hash, role, created_at) VALUES ('%s', '%s', '%s', GETDATE())",in1,in2,in3); astra_execute(db,q); needRefresh=true; showPopup=false;
}

void EksekusiSelesaiKembali(AstraDB *db) {
    if (strlen(currentTransID) == 0) return;
    char q[512];
    // 1. Tambah stok kembali
    sprintf(q, "UPDATE Barang SET stok_tersedia = stok_tersedia + (SELECT jumlah FROM TransaksiPeminjaman WHERE id='%s') WHERE barang_id = (SELECT barang_id FROM TransaksiPeminjaman WHERE id='%s')", currentTransID, currentTransID);
    astra_execute(db, q);
    // 2. Update status ke Kembali
    sprintf(q, "UPDATE TransaksiPeminjaman SET status='Kembali' WHERE id='%s'", currentTransID);
    astra_execute(db, q);

    showDendaPopup = false;
    needRefresh = true;
    memset(currentTransID, 0, 10);
}

// --- 6. TABEL RENDER ---

void DrawTableBarang(AstraDB *db, float x, float y, float w) {
    DrawHeader(x, y, w, "ID", "KODE", "NAMA BARANG", "STOK");
    for (int i = 0; i < countBarang; i++) {
        float ry = y + 40 + (i * 40);
        DrawText(listBarang[i].id, (int)x+10, (int)ry, 20, BLACK);
        DrawText(listBarang[i].kode, (int)x+100, (int)ry, 20, BLACK);
        DrawText(listBarang[i].nama, (int)x+300, (int)ry, 20, BLACK);
        DrawText(listBarang[i].stok, (int)x+550, (int)ry, 20, BLACK);
        
        if (DrawBtn(x+w-180, ry-5, 50, "HAPUS", COL_BTN_DELETE)) {
            char q[100]; sprintf(q, "DELETE FROM Barang WHERE barang_id=%s", listBarang[i].id);
            astra_execute(db, q); needRefresh = true;
        }
        if (DrawBtn(x+w-120, ry-5, 50, "EDIT", ORANGE)) {
             showPopup=true; strcpy(currentID, listBarang[i].id); 
             strcpy(in1, listBarang[i].kode); strcpy(in2, listBarang[i].nama); strcpy(in3, listBarang[i].stok);
             activeInput=1;
        }
        if (DrawBtn(x+w-60, ry-5, 50, "DETAIL", SKYBLUE)) { showDetailPopup = true; detailIndex = i; }
    }
}

void DrawTableVendor(AstraDB *db, float x, float y, float w) {
    DrawHeader(x, y, w, "ID", "NAMA", "ALAMAT", "TELP");
    for (int i = 0; i < countVendor; i++) {
        float ry = y + 40 + (i * 40);
        DrawText(listVendor[i].id, (int)x+10, (int)ry, 20, BLACK);
        DrawText(listVendor[i].nama, (int)x+100, (int)ry, 20, BLACK);
        DrawText(listVendor[i].alamat, (int)x+300, (int)ry, 20, BLACK);
        DrawText(listVendor[i].no_telp, (int)x+550, (int)ry, 20, BLACK);
        
        if (DrawBtn(x+w-180, ry-5, 50, "HAPUS", COL_BTN_DELETE)) {
            char q[100]; sprintf(q, "DELETE FROM Vendor WHERE vendor_id=%s", listVendor[i].id);
            astra_execute(db, q); needRefresh = true;
        }
        if (DrawBtn(x+w-120, ry-5, 50, "EDIT", ORANGE)) {
             showPopup=true; strcpy(currentID, listVendor[i].id); 
             strcpy(in1, listVendor[i].nama); strcpy(in2, listVendor[i].alamat); strcpy(in3, listVendor[i].no_telp);
             activeInput=1;
        }
        if (DrawBtn(x+w-60, ry-5, 50, "DETAIL", SKYBLUE)) { showDetailPopup = true; detailIndex = i; }
    }
}

void DrawTableRuangan(AstraDB *db, float x, float y, float w) {
    DrawHeader(x,y,w,"ID","RUANGAN","LOKASI",NULL);
    for(int i=0; i<countRuangan; i++) {
        float ry=y+40+(i*40);
        DrawText(listRuangan[i].id,(int)x+10,(int)ry,20,BLACK); DrawText(listRuangan[i].nama,(int)x+100,(int)ry,20,BLACK); DrawText(listRuangan[i].lokasi,(int)x+300,(int)ry,20,BLACK);
        
        if(DrawBtn(x+w-180,ry-5,50,"HAPUS",COL_BTN_DELETE)) { char q[100]; sprintf(q,"DELETE FROM Ruangan WHERE ruangan_id=%s",listRuangan[i].id); astra_execute(db,q); needRefresh=true; }
        if(DrawBtn(x+w-120,ry-5,50,"EDIT",ORANGE)) { showPopup=true; strcpy(currentID,listRuangan[i].id); strcpy(in1,listRuangan[i].nama); strcpy(in2,listRuangan[i].lokasi); memset(in3,0,100); activeInput=1; }
        if(DrawBtn(x+w-60, ry-5, 50, "DETAIL", SKYBLUE)) { showDetailPopup = true; detailIndex = i; }
    }
}

void DrawTableUser(AstraDB *db, float x, float y, float w) {
    DrawHeader(x,y,w,"ID","USERNAME","ROLE",NULL);
    for(int i=0; i<countUser; i++) {
        float ry=y+40+(i*40);
        DrawText(listUser[i].id,(int)x+10,(int)ry,20,BLACK); DrawText(listUser[i].username,(int)x+100,(int)ry,20,BLACK); DrawText(listUser[i].role,(int)x+300,(int)ry,20,BLACK);
        
        if(DrawBtn(x+w-180,ry-5,50,"HAPUS",COL_BTN_DELETE)) { char q[100]; sprintf(q,"DELETE FROM users WHERE user_id=%s",listUser[i].id); astra_execute(db,q); needRefresh=true; }
        if(DrawBtn(x+w-120,ry-5,50,"EDIT",ORANGE)) { showPopup=true; strcpy(currentID,listUser[i].id); strcpy(in1,listUser[i].username); strcpy(in2,""); strcpy(in3,listUser[i].role); activeInput=1; }
        if(DrawBtn(x+w-60, ry-5, 50, "DETAIL", SKYBLUE)) { showDetailPopup = true; detailIndex = i; }
    }
}

void DrawTablePeminjaman(AstraDB *db, float x, float y, float w) {
    DrawHPeminjam(x, y, w, "PEMINJAM", "BARANG", "TGL PENGAJUAN", "STATUS");
    for (int i = 0; i < countPinjam; i++) {
        float ry = y + 40 + (i * 45);
        DrawText(listPinjam[i].peminjam, (int)x+10, (int)ry, 20, BLACK);
        DrawText(listPinjam[i].barang, (int)x+150, (int)ry, 20, BLACK);
        
        bool telat = (listPinjam[i].hari_terlambat > 0);
        DrawText(listPinjam[i].tgl_rencana, (int)x+390, (int)ry, 20, telat?RED:BLACK);
        if(telat) DrawText(TextFormat("(+%d Hari)", listPinjam[i].hari_terlambat), (int)x+300, (int)ry+20, 12, RED);
        DrawText(listPinjam[i].status, (int)x+630, (int)ry, 20, BLUE);

        float btnX = x + w - 125; 

        if (strcmp(listPinjam[i].status, "Pending") == 0) {
            if (DrawBtn(btnX - 60, ry, 55, "ACC", GREEN)) {
                 char q[256]; sprintf(q, "UPDATE TransaksiPeminjaman SET status='Dipinjam' WHERE id='%s'", listPinjam[i].id); astra_execute(db, q);
                 needRefresh = true;
            }
            if (DrawBtn(btnX, ry, 55, "TOLAK", RED)) {
                char q[512];
                sprintf(q, "UPDATE Barang SET stok_tersedia = stok_tersedia + (SELECT jumlah FROM TransaksiPeminjaman WHERE id='%s') WHERE barang_id = (SELECT barang_id FROM TransaksiPeminjaman WHERE id='%s')", listPinjam[i].id, listPinjam[i].id);
                astra_execute(db, q);
                sprintf(q, "UPDATE TransaksiPeminjaman SET status='Ditolak' WHERE id='%s'", listPinjam[i].id);
                astra_execute(db, q); 
                needRefresh = true;
            }
        } 
        else if (strcmp(listPinjam[i].status, "Dipinjam") == 0) {
            if (DrawBtn(btnX - 55, ry, 110, "KEMBALIKAN", ORANGE)) {
                strcpy(currentTransID, listPinjam[i].id); 
                if (telat) {
                    showDendaPopup = true; 
                    sprintf(in1, "%d", listPinjam[i].hari_terlambat * 5000); 
                } else {
                    EksekusiSelesaiKembali(db);
                }
            }
        }
    }
}

// --- 7. MAIN RENDER ---

AppState DrawDashboardAdmin(AstraDB *db, Texture2D logo) {
    if (needRefresh) {
        LoadBarang(db); LoadVendor(db); LoadRuangan(db); LoadUser(db); LoadPeminjaman(db);
        needRefresh = false;
    }

    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    DrawRectangle(0, 0, 250, (int)sh, COL_SIDEBAR);
    if(logo.id>0) DrawTexturePro(logo, (Rectangle){0,0,(float)logo.width,(float)logo.height}, (Rectangle){25, 30, 200, logo.height*(200.0f/logo.width)}, (Vector2){0,0}, 0.0f, WHITE);
    
    const char *labels[] = {"DATA BARANG", "DATA VENDOR", "DATA USER", "DATA RUANGAN", "PEMINJAMAN"};
    AdminMenuState states[] = {MENU_BARANG, MENU_VENDOR, MENU_USERS, MENU_RUANGAN, MENU_PEMINJAMAN};

    for (int i = 0; i < 5; i++) {
        Rectangle btn = {20, 150 + (i * 60.0f), 210, 40};
        if (currentMenu == states[i]) DrawRectangleRec(btn, WHITE);
        if (CheckCollisionPointRec(GetMousePosition(), btn)) {
            DrawRectangleLinesEx(btn, 1, COL_ACCENT);
            if (IsMouseButtonPressed(0)) { currentMenu = states[i]; showDetailPopup = false; }
        }
        DrawText(labels[i], (int)btn.x + 20, (int)btn.y + 15, 16, BLACK);
    }
    
    Rectangle btnLogout = {20, sh - 80, 210, 40};
    bool hoverLogout = CheckCollisionPointRec(GetMousePosition(), btnLogout);
    if (hoverLogout) {
        DrawRectangleRec(btnLogout, WHITE);
        DrawRectangleLinesEx(btnLogout, 2, COL_BTN_LOGOUT);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { return STATE_MENU; }
    }
    DrawText("KELUAR", 40, sh-70, 18, hoverLogout ? COL_BTN_LOGOUT : RED);

    float cx = 270, cy = 100, cw = sw - 290;
    const char* pageTitle = "";
    switch(currentMenu) {
        case MENU_BARANG: pageTitle = "MASTER BARANG"; break;
        case MENU_VENDOR: pageTitle = "MASTER VENDOR"; break;
        case MENU_USERS:  pageTitle = "MASTER USERS"; break;
        case MENU_RUANGAN:pageTitle = "MASTER RUANGAN"; break;
        case MENU_PEMINJAMAN: pageTitle = "TRANSAKSI PEMINJAMAN"; break;
    }
    
    DrawText(pageTitle, (int)cx, 40, 30, BLACK);
    if (currentMenu != MENU_PEMINJAMAN) {
        if (DrawBtn(sw - 180, 30, 150, "+ TAMBAH", DARKGREEN)) {
            showPopup = true; strcpy(currentID,""); memset(in1,0,100); memset(in2,0,100); memset(in3,0,100); activeInput=1;
        }
    }

    DrawRectangleRec((Rectangle){cx, cy, cw, sh - 120}, COL_CONTENT);
    switch (currentMenu) {
        case MENU_BARANG: DrawTableBarang(db, cx+20, cy+20, cw-40); break;
        case MENU_VENDOR: DrawTableVendor(db, cx+20, cy+20, cw-40); break;
        case MENU_RUANGAN:DrawTableRuangan(db, cx+20, cy+20, cw-40); break;
        case MENU_USERS:  DrawTableUser(db, cx+20, cy+20, cw-40); break;
        case MENU_PEMINJAMAN: DrawTablePeminjaman(db, cx+20, cy+20, cw-40); break;
    }

    if (showPopup) {
        DrawRectangle(0, 0, (int)sw, (int)sh, (Color){0, 0, 0, 150});
        float bx = (sw - 500) / 2, by = (sh - 400) / 2;
        DrawRectangle((int)bx, (int)by, 500, 400, RAYWHITE);
        DrawText(strlen(currentID)>0?"EDIT DATA":"TAMBAH DATA", (int)bx+20, (int)by+20, 20, COL_ACCENT);
        const char *l1="", *l2="", *l3="";
        if(currentMenu==MENU_BARANG) { l1="Kode"; l2="Nama"; l3="Stok"; }
        else if(currentMenu==MENU_VENDOR) { l1="Vendor"; l2="Alamat"; l3="Telp"; }
        else if(currentMenu==MENU_USERS) { l1="Username"; l2="Password"; l3="Role"; }
        else { l1="Ruangan"; l2="Lokasi"; l3=""; }
        DrawInput(bx+20, by+80, l1, in1, 1);
        DrawInput(bx+20, by+130, l2, in2, 2);
        if(currentMenu!=MENU_RUANGAN) DrawInput(bx+20, by+180, l3, in3, 3);
        if(activeInput==1) HandleInput(in1, 49, true);
        if(activeInput==2) HandleInput(in2, 99, true);
        if(activeInput==3) HandleInput(in3, 49, true);
        if (DrawBtn(bx+20, by+330, 100, "SIMPAN", GREEN)) {
            if(currentMenu==MENU_BARANG) SaveBarang(db); else if(currentMenu==MENU_VENDOR) SaveVendor(db); else if(currentMenu==MENU_RUANGAN) SaveRuangan(db); else if(currentMenu==MENU_USERS) SaveUser(db);
        }
        if (DrawBtn(bx+140, by+330, 100, "BATAL", RED)) showPopup = false;
    }

    // --- POPUP DETAIL (DENGAN RELASI VIRTUAL) ---
    if (showDetailPopup && detailIndex >= 0) {
        DrawRectangle(0, 0, (int)sw, (int)sh, (Color){0, 0, 0, 180});
        float bx = (sw - 500) / 2, by = (sh - 400) / 2;
        DrawRectangleRec((Rectangle){bx, by, 500, 400}, RAYWHITE);
        DrawRectangleLinesEx((Rectangle){bx, by, 500, 400}, 2, COL_ACCENT);
        DrawText("DETAIL INFORMASI", (int)bx + 20, (int)by + 20, 24, COL_ACCENT);
        DrawLine((int)bx + 20, (int)by + 55, (int)bx + 480, (int)by + 55, LIGHTGRAY);
        float ty = by + 80;
        #define DRAW_ROW(lbl, val) { DrawText(lbl, (int)bx+30, (int)ty, 18, DARKGRAY); DrawText(val, (int)bx+180, (int)ty, 18, BLACK); ty+=35; }
        
        if (currentMenu == MENU_BARANG) { 
            DRAW_ROW("ID Barang:", listBarang[detailIndex].id); 
            DRAW_ROW("Kode:", listBarang[detailIndex].kode); 
            DRAW_ROW("Nama:", listBarang[detailIndex].nama); 
            DRAW_ROW("Stok:", listBarang[detailIndex].stok); 
            
            // --- PERBAIKAN RELASI VIRTUAL VENDOR ---
            const char* vMatch = "-"; // Default jika tidak ketemu
            
            for(int j = 0; j < countVendor; j++) {
                // Gunakan TextToLower agar pencarian tidak peduli huruf besar/kecil (Case-Insensitive)
                const char* itemLower = TextToLower(listBarang[detailIndex].nama);
                const char* vendorLower = TextToLower(listVendor[j].nama);
                
                // Cek apakah nama vendor ada di dalam nama barang
                // ATAU apakah nama barang mengandung bagian dari nama vendor
                if(strstr(itemLower, vendorLower) != NULL) {
                    vMatch = listVendor[j].nama;
                    break; 
                }
            }
            DRAW_ROW("Vendor:", vMatch);

            // --- PERBAIKAN RELASI VIRTUAL LOKASI ---
            const char* rMatch = "-";
            if (countRuangan > 0) {
                // Distribusi otomatis berdasarkan sisa bagi ID Barang dengan jumlah Ruangan
                int idNum = atoi(listBarang[detailIndex].id);
                rMatch = listRuangan[idNum % countRuangan].nama;
            } else {
                rMatch = "Gudang Utama";
            }
            DRAW_ROW("Lokasi:", rMatch);
        }
        else if (currentMenu == MENU_VENDOR) { DRAW_ROW("ID Vendor:", listVendor[detailIndex].id); DRAW_ROW("Nama:", listVendor[detailIndex].nama); DRAW_ROW("Alamat:", listVendor[detailIndex].alamat); DRAW_ROW("Telp:", listVendor[detailIndex].no_telp); }
        else if (currentMenu == MENU_RUANGAN) { DRAW_ROW("ID Ruangan:", listRuangan[detailIndex].id); DRAW_ROW("Nama:", listRuangan[detailIndex].nama); DRAW_ROW("Lokasi:", listRuangan[detailIndex].lokasi); }
        else if (currentMenu == MENU_USERS) { 
            DRAW_ROW("ID User:", listUser[detailIndex].id); 
            DRAW_ROW("Username:", listUser[detailIndex].username); 
            DRAW_ROW("Role:", listUser[detailIndex].role); 
            // TAMBAHKAN BARIS INI:
            DRAW_ROW("Password:", listUser[detailIndex].password); 
        }        else if (currentMenu == MENU_PEMINJAMAN) { DRAW_ROW("Peminjam:", listPinjam[detailIndex].peminjam); DRAW_ROW("Barang:", listPinjam[detailIndex].barang); DRAW_ROW("Status:", listPinjam[detailIndex].status); }
        
        if (DrawBtn(bx + 175, by + 340, 150, "TUTUP", COL_ACCENT)) { showDetailPopup = false; detailIndex = -1; }
    }

    if (showDendaPopup) {
        DrawRectangle(0, 0, (int)sw, (int)sh, (Color){0, 0, 0, 200}); 
        float bx = (sw - 500) / 2, by = (sh - 300) / 2;
        DrawRectangle((int)bx, (int)by, 500, 300, RAYWHITE);
        DrawRectangleLinesEx((Rectangle){bx, by, 500, 300}, 2, RED);
        DrawText("KONFIRMASI DENDA", (int)bx + 20, (int)by + 20, 20, RED);
        DrawText(TextFormat("Total Denda: Rp %s", in1), (int)bx + 20, (int)by + 80, 22, BLACK);
        if (DrawBtn(bx + 20, by + 220, 200, "SUDAH BAYAR", GREEN)) EksekusiSelesaiKembali(db);
        if (DrawBtn(bx + 240, by + 220, 100, "BATAL", LIGHTGRAY)) showDendaPopup = false;
    }

    return STATE_DASHBOARD_ADMIN;
}