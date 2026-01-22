// --- START OF FILE login.c (MODERN VERSION + REGISTER LINK) ---

#include "db_astra.h"
#include "raylib.h"
#include "common.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// --- PALET WARNA MODERN ---
#define COL_BG (Color){240, 240, 245, 255}
#define COL_CARD (Color){255, 253, 240, 255}
#define COL_SHADOW (Color){0, 0, 0, 30}
#define COL_INPUT_BG RAYWHITE
#define COL_BTN_BASE (Color){255, 150, 50, 255}
#define COL_BTN_HOVER (Color){255, 130, 20, 255}
#define COL_TEXT_LBL (Color){100, 100, 100, 255}
#define COL_ACCENT (Color){255, 165, 0, 255}
#define COL_LINK (Color){0, 102, 204, 255} // Biru Link

// Ukuran Layout
#define CARD_WIDTH 450
#define CARD_HEIGHT 600 // Sedikit dipertinggi untuk link
#define INPUT_WIDTH 360
#define INPUT_HEIGHT 45

// --- STATE GLOBAL ---
static char username[64] = "\0";
static char password[64] = "\0";
static int activeBox = 0;
static bool loginError = false;

// --- HELPER FUNCTIONS ---

static void Backspace(char *text)
{
    int len = strlen(text);
    if (len > 0)
        text[len - 1] = '\0';
}

static bool DrawInputBox(float x, float y, const char *label, char *text, bool isActive, bool isPassword)
{
    DrawText(label, (int)x, (int)(y - 25), 18, COL_TEXT_LBL);

    Rectangle rect = {x, y, INPUT_WIDTH, INPUT_HEIGHT};
    Vector2 mouse = GetMousePosition();
    bool isHover = CheckCollisionPointRec(mouse, rect);

    DrawRectangleRounded((Rectangle){x + 2, y + 2, INPUT_WIDTH, INPUT_HEIGHT}, 0.2f, 4, COL_SHADOW);
    DrawRectangleRounded(rect, 0.2f, 4, COL_INPUT_BG);

    if (isActive)
        DrawRectangleRoundedLines(rect, 0.2f, 4, COL_ACCENT);
    else
        DrawRectangleRoundedLines(rect, 0.2f, 4, LIGHTGRAY);

    char displayText[64];
    if (isPassword)
    {
        int len = strlen(text);
        for (int i = 0; i < len; i++)
            displayText[i] = '*';
        displayText[len] = '\0';
    }
    else
    {
        strcpy(displayText, text);
    }

    DrawText(displayText, (int)(x + 15), (int)(y + 12), 20, (Color){50, 50, 50, 255});

    return (isHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
}

// --- LOGIKA DATABASE ---
AppState CheckDatabaseLogin(const char *inputUser, const char *inputPass)
{
    AstraDB db;
    AppState nextState = STATE_LOGIN;

    if (astra_connect(&db, "project_astra"))
    {
        char query[256];
        sprintf(query, "SELECT user_id, role FROM users WHERE username='%s' AND password_hash='%s'", inputUser, inputPass);
        if (astra_query(&db, query))
        {
            if (SQLFetch(db.hStmt) == SQL_SUCCESS)
            {
                char idStr[10], roleStr[20];
                SQLGetData(db.hStmt, 1, SQL_C_CHAR, idStr, 10, NULL);
                SQLGetData(db.hStmt, 2, SQL_C_CHAR, roleStr, 20, NULL);
                g_currentUserId = atoi(idStr);
                if (strcmp(roleStr, "admin") == 0)
                    nextState = STATE_DASHBOARD_ADMIN;
                else
                    nextState = STATE_DASHBOARD_USER;
            }
        }
        astra_disconnect(&db);
    }
    else
    {
        loginError = true;
    }
    return nextState;
}

// --- TAMPILAN UTAMA ---
AppState DrawLoginScreen(Texture2D imgLogo)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    float startX = (sw - CARD_WIDTH) / 2.0f;
    float startY = (sh - CARD_HEIGHT) / 2.0f;
    float contentX = startX + (CARD_WIDTH - INPUT_WIDTH) / 2.0f;

    // --- LOGIKA INPUT KEYBOARD ---
    if (IsKeyPressed(KEY_TAB))
        activeBox = !activeBox;
    int key = GetCharPressed();
    char *target = (activeBox == 0) ? username : password;
    if (key > 0 || IsKeyPressed(KEY_BACKSPACE))
        loginError = false;
    while (key > 0)
    {
        if ((key >= 32) && (key <= 125) && (strlen(target) < 63))
        {
            int len = strlen(target);
            target[len] = (char)key;
            target[len + 1] = '\0';
        }
        key = GetCharPressed();
    }
    if (IsKeyDown(KEY_BACKSPACE) && IsKeyPressed(KEY_BACKSPACE))
        Backspace(target);

    // --- MENGGAMBAR UI ---

    // Background
    DrawRectangleGradientV(0, 0, sw, sh, WHITE, COL_BG);

    // Card + Shadow
    DrawRectangleRounded((Rectangle){startX + 10, startY + 10, CARD_WIDTH, CARD_HEIGHT}, 0.1f, 10, COL_SHADOW);
    DrawRectangleRounded((Rectangle){startX, startY, CARD_WIDTH, CARD_HEIGHT}, 0.1f, 10, COL_CARD);
    DrawRectangleRoundedLines((Rectangle){startX, startY, CARD_WIDTH, CARD_HEIGHT}, 0.1f, 10, BLACK);

    // Logo
    float logoY = startY + 50;
    if (imgLogo.id > 0)
    {
        float targetW = 220.0f;
        float scale = targetW / imgLogo.width;
        float logoH = imgLogo.height * scale;
        if (logoH > 120)
            logoH = 120;
        DrawTexturePro(imgLogo, (Rectangle){0, 0, (float)imgLogo.width, (float)imgLogo.height},
                       (Rectangle){startX + (CARD_WIDTH - targetW) / 2, logoY, targetW, logoH}, (Vector2){0, 0}, 0.0f, WHITE);
    }
    else
    {
        DrawText("BORROWBEE", (int)startX + 130, (int)logoY, 40, BROWN);
    }

    // Input Fields
    float inputY = startY + 200;
    if (DrawInputBox(contentX, inputY, "Username", username, (activeBox == 0), false))
        activeBox = 0;
    if (DrawInputBox(contentX, inputY + 90, "Password", password, (activeBox == 1), true))
        activeBox = 1;

    // Tombol Login
    float btnY = inputY + 180;
    Rectangle btnRect = {startX + (CARD_WIDTH - 200) / 2, btnY, 200, 50};
    Vector2 mouse = GetMousePosition();
    bool hoverBtn = CheckCollisionPointRec(mouse, btnRect);
    bool clickBtn = (hoverBtn && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || IsKeyPressed(KEY_ENTER);

    DrawRectangleRounded((Rectangle){btnRect.x + 2, btnRect.y + 4, btnRect.width, btnRect.height}, 0.3f, 6, COL_SHADOW);
    DrawRectangleRounded(btnRect, 0.3f, 6, hoverBtn ? COL_BTN_HOVER : COL_BTN_BASE);

    const char *txt = "MASUK";
    int txtSize = MeasureText(txt, 22);
    DrawText(txt, (int)(btnRect.x + (btnRect.width - txtSize) / 2), (int)(btnRect.y + 14), 22, WHITE);

    // --- LINK REGISTER DI SINI ---
    const char *txtLink = "Belum punya akun? Daftar";
    int linkW = MeasureText(txtLink, 16);
    int linkX = (int)(startX + (CARD_WIDTH - linkW) / 2);
    int linkY = (int)(btnY + 70);

    // Deteksi Hover pada Text Link
    Rectangle linkRect = {linkX, linkY, linkW, 20};
    bool hoverLink = CheckCollisionPointRec(mouse, linkRect);

    DrawText(txtLink, linkX, linkY, 16, hoverLink ? SKYBLUE : COL_LINK);

    // Garis bawah jika di-hover (efek link)
    if (hoverLink)
    {
        DrawLine(linkX, linkY + 18, linkX + linkW, linkY + 18, SKYBLUE);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            // Reset form sebelum pindah
            username[0] = '\0';
            password[0] = '\0';
            loginError = false;
            return STATE_REGISTER; // PINDAH KE REGISTER
        }
    }

    // Pesan Error
    if (loginError)
    {
        const char *err = "Username atau Password Salah!";
        int errW = MeasureText(err, 16);
        DrawText(err, (int)(startX + (CARD_WIDTH - errW) / 2), (int)(linkY + 30), 16, RED);
    }

    // Eksekusi Login
    if (clickBtn)
    {
        AppState result = CheckDatabaseLogin(username, password);
        if (result != STATE_LOGIN)
        {
            username[0] = '\0';
            password[0] = '\0';
            loginError = false;
            return result;
        }
        else
        {
            loginError = true;
        }
    }

    if (IsKeyPressed(KEY_GRAVE))
    {
        username[0] = '\0';
        password[0] = '\0';
        return STATE_MENU;
    }

    return STATE_LOGIN;
}