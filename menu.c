// --- START OF FILE menu.c ---

#include "common.h"
#include <stdio.h>

// --- 1. KONFIGURASI WARNA & UKURAN ---
#define COL_BG          RAYWHITE                    // Putih
#define COL_STRIP       (Color){235, 228, 197, 255} // Beige (Bawah)
#define COL_SHADOW      (Color){0, 0, 0, 30}        // Bayangan

// Warna Tombol
#define COL_BTN_LOGIN   (Color){235, 228, 197, 255} // Beige
#define COL_BTN_REG     (Color){0, 121, 241, 255}   // Biru
#define COL_BTN_EXIT    (Color){255, 59, 48, 255}   // Merah

#define BTN_WIDTH  280.0f
#define BTN_HEIGHT 55.0f
#define SPACING    25.0f

// --- 2. FUNGSI PEMBANTU (MENGGAMBAR TOMBOL) ---
void DrawButtonSimple(Rectangle rect, const char* text, Color color, Color textColor, bool isActive) 
{
    // Efek Hover: Geser sedikit kalau dipilih
    if (isActive) {
        rect.x -= 2;
        rect.y -= 2;
    }

    // 1. Gambar Bayangan (Kotak abu-abu di belakang)
    DrawRectangleRounded((Rectangle){rect.x + 4, rect.y + 4, rect.width, rect.height}, 0.3f, 6, COL_SHADOW);

    // 2. Gambar Tombol Utama
    DrawRectangleRounded(rect, 0.3f, 6, color);

    // 3. Garis Pinggir (Jika dipilih)
    if (isActive) {
        DrawRectangleRoundedLines(rect, 0.3f, 6, ORANGE);
    }

    // 4. Teks di Tengah
    int fontSize = 20;
    int textWidth = MeasureText(text, fontSize);
    DrawText(text, 
             (int)(rect.x + (rect.width - textWidth) / 2), 
             (int)(rect.y + (rect.height - fontSize) / 2), 
             fontSize, textColor);
}

// --- 3. FUNGSI UTAMA MENU ---
AppState DrawMenuScreen(Texture2D logo)
{
    // Variabel Pilihan (0=Login, 1=Register, 2=Exit)
    static int selected = 0; 

    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    Vector2 mouse = GetMousePosition();

    // -- SETUP POSISI TOMBOL DI TENGAH --
    float startY = (sh / 2.0f) + 2.0f; 
    float btnX = (sw - BTN_WIDTH) / 2.0f;

    Rectangle boxLogin = {btnX, startY, BTN_WIDTH, BTN_HEIGHT};
    Rectangle boxReg   = {btnX, startY + BTN_HEIGHT + SPACING, BTN_WIDTH, BTN_HEIGHT};
    Rectangle boxExit  = {btnX, startY + (BTN_HEIGHT + SPACING) * 2, BTN_WIDTH, BTN_HEIGHT};

    // -- LOGIKA INPUT (MOUSE & KEYBOARD) --
    
    // Mouse Hover
    if (CheckCollisionPointRec(mouse, boxLogin)) selected = 0;
    else if (CheckCollisionPointRec(mouse, boxReg))   selected = 1;
    else if (CheckCollisionPointRec(mouse, boxExit))  selected = 2;

    // Keyboard Arrow
    if (IsKeyPressed(KEY_DOWN)) { selected++; if (selected > 2) selected = 0; }
    if (IsKeyPressed(KEY_UP))   { selected--; if (selected < 0) selected = 2; }

    // -- MENGGAMBAR (DRAWING) --

    // 1. Background Putih
    DrawRectangle(0, 0, (int)sw, (int)sh, COL_BG);
    
    // 2. Hiasan Bawah (Strip Beige)
    DrawRectangle(0, (int)(sh * 0.85f), (int)sw, (int)(sh * 0.15f), COL_STRIP);

    // 3. Logo (Diam di Tengah Atas)
    if (logo.id > 0) {
        float scale = 300.0f / logo.width;
        float logoH = logo.height * scale;
        
        // Gambar logo
        DrawTexturePro(logo, (Rectangle){0,0,(float)logo.width,(float)logo.height},
            (Rectangle){(sw - 300)/2, startY - logoH - 50, 300, logoH}, 
            (Vector2){0,0}, 0.0f, WHITE);
    } else {
        // Teks cadangan jika logo tidak ada
        DrawText("BORROWBEE", (int)((sw - 200)/2), (int)(startY - 100), 40, BROWN);
    }

    // 4. Gambar Tombol (Panggil Fungsi Pembantu)
    DrawButtonSimple(boxLogin, "MASUK", COL_BTN_LOGIN, BLACK, (selected == 0));
    DrawButtonSimple(boxReg,   "DAFTAR AKUN", COL_BTN_REG, WHITE, (selected == 1));
    DrawButtonSimple(boxExit,  "KELUAR", COL_BTN_EXIT, WHITE, (selected == 2));

    // -- EKSEKUSI KLIK / ENTER --
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_ENTER)) {
        
        // Validasi ekstra: Pastikan mouse benar-benar di atas tombol jika pakai klik
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (selected == 0 && !CheckCollisionPointRec(mouse, boxLogin)) return STATE_MENU;
            if (selected == 1 && !CheckCollisionPointRec(mouse, boxReg))   return STATE_MENU;
            if (selected == 2 && !CheckCollisionPointRec(mouse, boxExit))  return STATE_MENU;
        }

        // Pindah Halaman
        if (selected == 0) return STATE_LOGIN;
        if (selected == 1) return STATE_REGISTER;
        if (selected == 2) return STATE_EXIT;
    }

    return STATE_MENU;
}