// --- START OF FILE register.c (MODERN VERSION) ---

#include "common.h"
#include "db_astra.h" 
#include "raylib.h"
#include <string.h>
#include <stdio.h>

// --- 1. PALET WARNA MODERN ---
#define COL_BG_GRAD1    (Color){245, 245, 250, 255} // Putih Kebiruan
#define COL_BG_GRAD2    (Color){220, 220, 230, 255} // Abu-abu Soft
#define COL_PANEL_LEFT  (Color){255, 248, 220, 255} // Cream Lembut
#define COL_PANEL_RIGHT (Color){255, 255, 255, 255} // Putih Murni

#define COL_INPUT_BG    (Color){245, 245, 245, 255} // Abu Sangat Terang
#define COL_INPUT_ACT   (Color){255, 255, 255, 255} // Putih
#define COL_BORDER_ACT  (Color){255, 165, 0, 255}   // Oranye
#define COL_SHADOW      (Color){0, 0, 0, 20}        // Bayangan Transparan

#define COL_BTN_REG     (Color){30, 144, 255, 255}  // Biru Dodger
#define COL_BTN_HOVER   (Color){0, 100, 200, 255}   // Biru Gelap
#define COL_BTN_BACK    (Color){220, 50, 50, 255}   // Merah
#define COL_TEXT_LBL    (Color){80, 80, 80, 255}    // Abu Gelap

// --- 2. STATE GLOBAL ---
extern AstraDB g_db; 

static char username[64] = "";
static char email[64]    = "";
static char password[64] = "";
static char confirm[64]  = "";
static char adminToken[64] = "";

static int activeBox = 0;    
static int selectedRole = 1; 
static bool passMismatch = false;
static bool tokenMismatch = false;

const char *SECRET_ADMIN_CODE = "ADMIN123";

// --- 3. HELPER FUNCTIONS ---

static void Backspace(char *text) {
    int len = strlen(text);
    if (len > 0) text[len - 1] = '\0';
}

static void ResetRegisterFields() {
    username[0] = '\0'; email[0] = '\0';
    password[0] = '\0'; confirm[0] = '\0';
    adminToken[0] = '\0';
    activeBox = 0; selectedRole = 1;
    passMismatch = false; tokenMismatch = false;
}

// Input Modern (Rounded + Shadow)
static bool DrawModernInput(float x, float y, float w, const char* label, char* buffer, bool isPassword, bool isError, int id) {
    bool isActive = (activeBox == id);

    // Label
    DrawText(label, (int)x, (int)(y - 25), 16, COL_TEXT_LBL);
    
    Rectangle rect = {x, y, w, 45}; // Sedikit lebih tinggi biar lega
    
    // Shadow Halus
    DrawRectangleRounded((Rectangle){x+2, y+2, w, 45}, 0.2f, 6, COL_SHADOW);

    // Background Input
    DrawRectangleRounded(rect, 0.2f, 6, isActive ? COL_INPUT_ACT : COL_INPUT_BG);

    // Border (Hanya jika aktif/error)
    if (isError) DrawRectangleRoundedLines(rect, 0.2f, 6, RED);
    else if (isActive) DrawRectangleRoundedLines(rect, 0.2f, 6, COL_BORDER_ACT);
    else DrawRectangleRoundedLines(rect, 0.2f, 6, LIGHTGRAY); // Border tipis saat diam

    // Teks
    char displayText[64];
    if (isPassword) {
        int len = strlen(buffer);
        for(int i=0; i<len; i++) displayText[i] = '*';
        displayText[len] = '\0';
    } else {
        strcpy(displayText, buffer);
    }
    DrawText(displayText, (int)(x + 15), (int)(y + 12), 20, BLACK);

    return (CheckCollisionPointRec(GetMousePosition(), rect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
}

// Radio Button Modern
static bool DrawModernRadio(float x, float y, const char* label, bool isSelected) {
    Vector2 center = {x + 10, y + 10};
    DrawCircleV(center, 9, LIGHTGRAY); // Outline Luar
    DrawCircleV(center, 7, WHITE);     // Background Putih
    
    if (isSelected) DrawCircleV(center, 5, DARKGRAY); // Isi Hitam
    
    DrawText(label, (int)(x + 28), (int)(y), 18, BLACK);
    Rectangle touch = {x, y - 5, 100, 30};
    return (CheckCollisionPointRec(GetMousePosition(), touch) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
}

// Tombol Modern (Rounded + Hover Effect)
static bool DrawModernBtn(float x, float y, float w, const char* text, Color baseCol, Color hoverCol, int id) {
    Rectangle rect = {x, y, w, 45};
    bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
    bool focus = (activeBox == id);

    // Shadow Tombol
    DrawRectangleRounded((Rectangle){x+3, y+3, w, 45}, 0.3f, 6, COL_SHADOW);

    // Tombol Utama
    Color drawCol = (hover || focus) ? hoverCol : baseCol;
    DrawRectangleRounded(rect, 0.3f, 6, drawCol);
    
    // Outline Fokus Keyboard
    if (focus) DrawRectangleRoundedLines(rect, 0.3f, 6, ORANGE);

    int txtW = MeasureText(text, 20);
    DrawText(text, (int)(x + (w - txtW)/2), (int)(y + 12), 20, WHITE);

    if (hover) activeBox = id;
    return (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || (focus && IsKeyPressed(KEY_ENTER));
}

// --- 4. MAIN FUNCTION ---

AppState DrawRegisterScreen(Texture2D imgLogo)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // -- KEYBOARD LOGIC --
    if (IsKeyPressed(KEY_TAB)) {
        activeBox++;
        if (activeBox > 7) activeBox = 0;
        if (selectedRole == 1 && activeBox == 5) activeBox = 6; 
    }

    bool isTyping = (activeBox >= 0 && activeBox <= 3) || (activeBox == 5);
    if (isTyping) {
        int key = GetCharPressed();
        char *target = NULL;
        if (activeBox == 0) target = username;
        else if (activeBox == 1) target = email;
        else if (activeBox == 2) target = password;
        else if (activeBox == 3) { target = confirm; passMismatch = false; }
        else if (activeBox == 5) { target = adminToken; tokenMismatch = false; }

        if (target) {
            while (key > 0) {
                if ((key >= 32) && (key <= 125) && (strlen(target) < 63)) {
                    int len = strlen(target);
                    target[len] = (char)key; target[len + 1] = '\0';
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) Backspace(target);
        }
    }
    
    if (activeBox == 4) {
        if (IsKeyPressed(KEY_RIGHT)) selectedRole = 0;
        if (IsKeyPressed(KEY_LEFT)) selectedRole = 1;
    }

    // -- DRAWING --
    
    // Background Gradient
    DrawRectangleGradientV(0, 0, sw, sh, COL_BG_GRAD1, COL_BG_GRAD2);

    // Setup Card Layout
    float cardW = 900;
    float cardH = 550;
    float startX = (sw - cardW) / 2.0f;
    float startY = (sh - cardH) / 2.0f;

    Rectangle leftPanel = {startX, startY, cardW / 2, cardH};
    Rectangle rightPanel = {startX + (cardW / 2), startY, cardW / 2, cardH};

    // Shadow Card Utama (Keren!)
    DrawRectangleRounded((Rectangle){startX+10, startY+10, cardW, cardH}, 0.05f, 10, COL_SHADOW);

    // Panel Kiri (Beige + Logo)
    DrawRectangleRec(leftPanel, COL_PANEL_LEFT); // Beige
    
    if (imgLogo.id > 0) {
        float logoW = 220.0f;
        float scale = logoW / imgLogo.width;
        DrawTexturePro(imgLogo, (Rectangle){0,0,(float)imgLogo.width,(float)imgLogo.height},
            (Rectangle){leftPanel.x + (leftPanel.width - 220)/2, startY + (cardH - imgLogo.height*scale)/2, 220, imgLogo.height*scale},
            (Vector2){0, 0}, 0.0f, WHITE);
    } else {
        DrawText("BorrowBee", (int)(leftPanel.x + 120), (int)(startY + 250), 40, BROWN);
    }

    // Panel Kanan (Putih Bersih)
    DrawRectangleRec(rightPanel, COL_PANEL_RIGHT);

    float fieldW = rightPanel.width * 0.85f;
    float formX = rightPanel.x + (rightPanel.width - fieldW) / 2;
    float currentY = startY + 50;
    float gap = 75; // Lebih renggang biar rapi

    // Inputs Loop
    const char* labels[] = {"Username", "Email", "Password", "Konfirmasi Password"};
    char* buffers[] = {username, email, password, confirm};

    for(int i=0; i<4; i++) {
        if (DrawModernInput(formX, currentY, fieldW, labels[i], buffers[i], (i>=2), (i==3 && passMismatch), i))
            activeBox = i;
        if (i==3 && passMismatch) DrawText("Password tidak sama!", (int)formX, (int)(currentY + 50), 12, RED);
        currentY += gap;
    }

    // Role
    currentY -= 20;
    if (DrawModernRadio(formX, currentY, "Peminjam", (selectedRole == 1))) { selectedRole = 1; activeBox = 4; }
    if (DrawModernRadio(formX + 130, currentY, "Admin", (selectedRole == 0))) { selectedRole = 0; activeBox = 4; }

    currentY += 50;

    // Token Admin
    if (selectedRole == 0) {
        if (DrawModernInput(formX, currentY, fieldW, "Admin Code", adminToken, true, tokenMismatch, 5)) activeBox = 5;
        if (tokenMismatch) DrawText("Kode Salah!", (int)(formX+fieldW-90), (int)(currentY-25), 12, RED);
        currentY += gap;
    } else {
        currentY += 15;
    }

    // Tombol Register
    if (DrawModernBtn(formX, currentY, fieldW, "DAFTAR SEKARANG", COL_BTN_REG, COL_BTN_HOVER, 6)) {
        bool valid = true;
        if (strlen(username) == 0 || strlen(password) == 0) valid = false;
        if (strcmp(password, confirm) != 0) { passMismatch = true; valid = false; }
        if (selectedRole == 0 && strcmp(adminToken, SECRET_ADMIN_CODE) != 0) { tokenMismatch = true; valid = false; }

        if (valid) {
            AstraDB dbTemp;
            if (astra_connect(&dbTemp, "borrowbee;UID=lat;PWD=lat123")) {
                int res = astra_register(&dbTemp, username, email, password, (selectedRole==0?"admin":"peminjam"));
                if (res == 1) {
                    astra_disconnect(&dbTemp);
                    ResetRegisterFields();
                    return STATE_LOGIN;
                }
                astra_disconnect(&dbTemp);
            }
        }
    }

    // Tombol Kembali
    if (DrawModernBtn(formX, currentY + 60, fieldW, "KEMBALI", COL_BTN_BACK, MAROON, 7)) {
        ResetRegisterFields();
        return STATE_MENU;
    }

    // Shortcut ESC
    if (IsKeyPressed(KEY_GRAVE) || IsKeyPressed(KEY_ESCAPE)) {
        ResetRegisterFields();
        return STATE_MENU;
    }

    return STATE_REGISTER;
}