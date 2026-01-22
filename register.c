// --- START OF FILE register.c (FULL NAME + GMAIL VALIDATION) ---

#include "common.h"
#include "db_astra.h"
#include "raylib.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h> // Untuk tolower

// --- 1. PALET WARNA MODERN ---
#define COL_BG_GRAD1 (Color){245, 245, 250, 255}
#define COL_BG_GRAD2 (Color){220, 220, 230, 255}
#define COL_PANEL_LEFT (Color){255, 248, 220, 255}
#define COL_PANEL_RIGHT (Color){255, 255, 255, 255}

#define COL_INPUT_BG (Color){245, 245, 245, 255}
#define COL_INPUT_ACT (Color){255, 255, 255, 255}
#define COL_BORDER_ACT (Color){255, 165, 0, 255}
#define COL_SHADOW (Color){0, 0, 0, 20}

#define COL_BTN_REG (Color){30, 144, 255, 255}
#define COL_BTN_HOVER (Color){0, 100, 200, 255}
#define COL_BTN_BACK (Color){220, 50, 50, 255}
#define COL_TEXT_LBL (Color){80, 80, 80, 255}

// --- 2. STATE GLOBAL ---
extern AstraDB g_db;

static char fullname[64] = ""; // BUFFER BARU: Nama Lengkap
static char username[64] = "";
static char email[64] = "";
static char password[64] = "";
static char confirm[64] = "";

static int activeBox = 0;
static int selectedRole = 1;
static bool passMismatch = false;
static bool emailError = false; // ERROR STATE: Jika email bukan gmail

// -- STATE KHUSUS OTP --
static bool showOtpModal = false;
static char inputOtp[5] = "";
static int otpCursor = 0;
static bool otpError = false;

// --- 3. HELPER FUNCTIONS ---

static void Backspace(char *text)
{
    int len = strlen(text);
    if (len > 0)
        text[len - 1] = '\0';
}

static void ResetRegisterFields()
{
    fullname[0] = '\0'; // Reset Nama
    username[0] = '\0';
    email[0] = '\0';
    password[0] = '\0';
    confirm[0] = '\0';
    activeBox = 0;
    selectedRole = 1;
    passMismatch = false;
    emailError = false;

    // Reset OTP
    showOtpModal = false;
    memset(inputOtp, 0, 5);
    otpCursor = 0;
    otpError = false;
}

// Fungsi Cek Gmail
static bool IsGmail(const char *email)
{
    if (strlen(email) < 11)
        return false; // Minimal panjang "@gmail.com" + 1 karakter

    // Ambil 10 karakter terakhir
    const char *suffix = email + strlen(email) - 10;

    // Bandingkan (Case Insensitive logic sederhana)
    // Agar aman kita anggap user input huruf kecil semua atau kita bandingkan string literal
    // Tapi strcmp itu case sensitive. Untuk amannya kita cek string persis "@gmail.com"
    // User harus input persis atau kita bisa convert dulu. Di sini kita pakai strcmp biasa.
    return (strcmp(suffix, "@gmail.com") == 0);
}

// Input Modern
static bool DrawModernInput(float x, float y, float w, const char *label, char *buffer, bool isPassword, bool isError, int id)
{
    bool isActive = (activeBox == id);

    DrawText(label, (int)x, (int)(y - 20), 14, COL_TEXT_LBL); // Label sedikit diperkecil & digeser
    Rectangle rect = {x, y, w, 40};                           // Tinggi input sedikit diperkecil biar muat 5 baris

    DrawRectangleRounded((Rectangle){x + 2, y + 2, w, 40}, 0.2f, 6, COL_SHADOW);
    DrawRectangleRounded(rect, 0.2f, 6, isActive ? COL_INPUT_ACT : COL_INPUT_BG);

    if (isError)
        DrawRectangleRoundedLines(rect, 0.2f, 6, RED);
    else if (isActive)
        DrawRectangleRoundedLines(rect, 0.2f, 6, COL_BORDER_ACT);
    else
        DrawRectangleRoundedLines(rect, 0.2f, 6, LIGHTGRAY);

    char displayText[64];
    if (isPassword)
    {
        int len = strlen(buffer);
        for (int i = 0; i < len; i++)
            displayText[i] = '*';
        displayText[len] = '\0';
    }
    else
    {
        strcpy(displayText, buffer);
    }
    DrawText(displayText, (int)(x + 10), (int)(y + 10), 20, BLACK);

    return (CheckCollisionPointRec(GetMousePosition(), rect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
}

static bool DrawModernRadio(float x, float y, const char *label, bool isSelected)
{
    Vector2 center = {x + 10, y + 10};
    DrawCircleV(center, 9, LIGHTGRAY);
    DrawCircleV(center, 7, WHITE);
    if (isSelected)
        DrawCircleV(center, 5, DARKGRAY);

    DrawText(label, (int)(x + 28), (int)(y), 18, BLACK);
    Rectangle touch = {x, y - 5, 100, 30};
    return (CheckCollisionPointRec(GetMousePosition(), touch) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
}

static bool DrawModernBtn(float x, float y, float w, const char *text, Color baseCol, Color hoverCol, int id)
{
    Rectangle rect = {x, y, w, 45};
    bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
    bool focus = (activeBox == id);

    DrawRectangleRounded((Rectangle){x + 3, y + 3, w, 45}, 0.3f, 6, COL_SHADOW);
    DrawRectangleRounded(rect, 0.3f, 6, (hover || focus) ? hoverCol : baseCol);

    if (focus)
        DrawRectangleRoundedLines(rect, 0.3f, 6, ORANGE);

    int txtW = MeasureText(text, 20);
    DrawText(text, (int)(x + (w - txtW) / 2), (int)(y + 12), 20, WHITE);

    if (hover)
        activeBox = id;
    return (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || (focus && IsKeyPressed(KEY_ENTER));
}

// --- 4. MODAL OTP LOGIC (SAMA SEPERTI SEBELUMNYA) ---
static int HandleOtpModal(int screenW, int screenH)
{
    DrawRectangle(0, 0, screenW, screenH, (Color){0, 0, 0, 150});
    float mW = 400;
    float mH = 300;
    float mX = (screenW - mW) / 2;
    float mY = (screenH - mH) / 2;

    DrawRectangleRounded((Rectangle){mX, mY, mW, mH}, 0.1f, 10, WHITE);
    DrawRectangleRoundedLines((Rectangle){mX, mY, mW, mH}, 0.1f, 10, LIGHTGRAY);

    DrawText("VERIFIKASI ADMIN", (int)mX + 100, (int)mY + 30, 20, DARKGRAY);
    DrawText("Masukkan kode 4 digit dari Admin", (int)mX + 60, (int)mY + 60, 16, GRAY);

    int key = GetKeyPressed();
    if (otpCursor < 4)
    {
        if ((key >= KEY_ZERO && key <= KEY_NINE) || (key >= KEY_KP_0 && key <= KEY_KP_9))
        {
            int digit = (key >= KEY_ZERO && key <= KEY_NINE) ? (key - KEY_ZERO) : (key - KEY_KP_0);
            inputOtp[otpCursor] = (char)(digit + '0');
            otpCursor++;
            otpError = false;
        }
    }
    if (key == KEY_BACKSPACE && otpCursor > 0)
    {
        otpCursor--;
        inputOtp[otpCursor] = '\0';
        otpError = false;
    }

    int boxSize = 50;
    int gap = 15;
    int startBoxX = (int)(mX + (mW - (4 * boxSize + 3 * gap)) / 2);
    int startBoxY = (int)(mY + 110);

    for (int i = 0; i < 4; i++)
    {
        Rectangle box = {(float)(startBoxX + i * (boxSize + gap)), (float)startBoxY, (float)boxSize, (float)boxSize};
        Color border = LIGHTGRAY;
        if (otpError)
            border = RED;
        else if (i == otpCursor)
            border = BLUE;
        DrawRectangleRec(box, COL_INPUT_BG);
        DrawRectangleLinesEx(box, 2, border);
        if (i < otpCursor)
        {
            char numStr[2] = {inputOtp[i], '\0'};
            int textW = MeasureText(numStr, 30);
            DrawText(numStr, (int)(box.x + (boxSize - textW) / 2), (int)(box.y + 10), 30, BLACK);
        }
    }

    if (otpError)
        DrawText("Kode Salah / Sudah Terpakai!", (int)mX + 90, (int)mY + 175, 16, RED);

    Rectangle btnVerif = {mX + 50, mY + 220, 130, 45};
    Rectangle btnCancel = {mX + 220, mY + 220, 130, 45};
    bool hoverVerif = CheckCollisionPointRec(GetMousePosition(), btnVerif);
    DrawRectangleRounded(btnVerif, 0.3f, 6, hoverVerif ? COL_BTN_HOVER : COL_BTN_REG);
    DrawText("VERIFIKASI", (int)btnVerif.x + 20, (int)btnVerif.y + 12, 16, WHITE);
    bool hoverCancel = CheckCollisionPointRec(GetMousePosition(), btnCancel);
    DrawRectangleRounded(btnCancel, 0.3f, 6, hoverCancel ? MAROON : COL_BTN_BACK);
    DrawText("BATAL", (int)btnCancel.x + 40, (int)btnCancel.y + 12, 16, WHITE);

    bool submit = (hoverVerif && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || IsKeyPressed(KEY_ENTER);
    if (submit)
    {
        if (astra_check_token(&g_db, inputOtp))
        {
            astra_use_token(&g_db, inputOtp);
            return 1;
        }
        else
        {
            otpError = true;
        }
    }
    if ((hoverCancel && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) || IsKeyPressed(KEY_ESCAPE))
        return -1;
    return 0;
}

// --- 5. MAIN FUNCTION ---

// --- FILE register.c (FIXED LAYOUT) ---

// --- FILE register.c (FIXED OVERFLOW LAYOUT) ---

AppState DrawRegisterScreen(Texture2D imgLogo)
{
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // === LOGIKA MODAL OTP (TIDAK BERUBAH) ===
    if (showOtpModal)
    {
        int otpResult = HandleOtpModal(sw, sh);
        if (otpResult == 1)
        {
            int res = astra_register(&g_db, fullname, username, email, password, "admin");
            if (res == 1)
            {
                ResetRegisterFields();
                return STATE_LOGIN;
            }
            else
            {
                showOtpModal = false;
            }
        }
        else if (otpResult == -1)
        {
            showOtpModal = false;
            memset(inputOtp, 0, 5);
        }
        return STATE_REGISTER;
    }
    // ========================================

    // -- KEYBOARD LOGIC --
    if (IsKeyPressed(KEY_TAB))
    {
        activeBox++;
        if (activeBox > 5)
            activeBox = 0;
    }

    bool isTyping = (activeBox >= 0 && activeBox <= 4);
    if (isTyping)
    {
        int key = GetCharPressed();
        char *target = NULL;
        if (activeBox == 0)
            target = fullname;
        else if (activeBox == 1)
            target = username;
        else if (activeBox == 2)
        {
            target = email;
            emailError = false;
        }
        else if (activeBox == 3)
            target = password;
        else if (activeBox == 4)
        {
            target = confirm;
            passMismatch = false;
        }

        if (target)
        {
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
            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE))
                Backspace(target);
        }
    }

    if (activeBox == 5)
    {
        if (IsKeyPressed(KEY_RIGHT))
            selectedRole = 0;
        if (IsKeyPressed(KEY_LEFT))
            selectedRole = 1;
    }

    // -- DRAWING --
    DrawRectangleGradientV(0, 0, sw, sh, COL_BG_GRAD1, COL_BG_GRAD2);

    // --- PERBAIKAN 1: PERBESAR TINGGI KARTU ---
    float cardW = 900;
    float cardH = 620; // Diubah dari 550 menjadi 620 agar tombol merah masuk

    float startX = (sw - cardW) / 2.0f;
    float startY = (sh - cardH) / 2.0f; // Posisi Y otomatis menyesuaikan tengah

    Rectangle leftPanel = {startX, startY, cardW / 2, cardH};
    Rectangle rightPanel = {startX + (cardW / 2), startY, cardW / 2, cardH};

    DrawRectangleRounded((Rectangle){startX + 10, startY + 10, cardW, cardH}, 0.05f, 10, COL_SHADOW);
    DrawRectangleRec(leftPanel, COL_PANEL_LEFT);

    if (imgLogo.id > 0)
    {
        float logoW = 220.0f;
        float scale = logoW / imgLogo.width;
        // Logo ditaruh di tengah panel kiri
        DrawTexturePro(imgLogo, (Rectangle){0, 0, (float)imgLogo.width, (float)imgLogo.height},
                       (Rectangle){leftPanel.x + (leftPanel.width - 220) / 2, startY + (cardH - imgLogo.height * scale) / 2, 220, imgLogo.height * scale}, (Vector2){0, 0}, 0.0f, WHITE);
    }
    else
    {
        DrawText("BorrowBee", (int)(leftPanel.x + 120), (int)(startY + (cardH / 2) - 20), 40, BROWN);
    }

    DrawRectangleRec(rightPanel, COL_PANEL_RIGHT);

    // --- LOGIKA LAYOUT FORM ---
    float fieldW = rightPanel.width * 0.85f;
    float formX = rightPanel.x + (rightPanel.width - fieldW) / 2;

    // Mulai menggambar sedikit lebih ke atas
    float currentY = startY + 35;

    // --- PERBAIKAN 2: ATUR JARAK ANTAR INPUT ---
    // 75 pixel cukup aman: tidak nabrak error, tapi juga tidak terlalu boros tempat
    float gap = 75;

    const char *labels[] = {"Nama Lengkap", "Username", "Email", "Password", "Konfirmasi Password"};
    char *buffers[] = {fullname, username, email, password, confirm};

    for (int i = 0; i < 5; i++)
    {
        bool isPass = (i >= 3);
        bool isErr = false;

        if (i == 4 && passMismatch)
            isErr = true;
        if (i == 2 && emailError)
            isErr = true;

        if (DrawModernInput(formX, currentY, fieldW, labels[i], buffers[i], isPass, isErr, i))
            activeBox = i;

        // Posisi Pesan Error (sedikit di bawah kotak input)
        if (i == 4 && passMismatch)
            DrawText("Password tidak sama!", (int)formX, (int)(currentY + 45), 12, RED);
        if (i == 2 && emailError)
            DrawText("Email harus @gmail.com", (int)formX, (int)(currentY + 45), 12, RED);

        currentY += gap;
    }

    // Role Selection (Radio Button)
    // Geser sedikit ke atas (mendekati input terakhir)
    currentY -= 5;

    if (DrawModernRadio(formX, currentY, "Peminjam", (selectedRole == 1)))
    {
        selectedRole = 1;
        activeBox = 5;
    }
    if (DrawModernRadio(formX + 130, currentY, "Admin (Token)", (selectedRole == 0)))
    {
        selectedRole = 0;
        activeBox = 5;
    }

    // Geser ke bawah untuk tombol
    currentY += 45;

    // Tombol Register
    if (DrawModernBtn(formX, currentY, fieldW, "DAFTAR SEKARANG", COL_BTN_REG, COL_BTN_HOVER, 6))
    {
        bool valid = true;
        if (strlen(fullname) == 0 || strlen(username) == 0 || strlen(email) == 0 || strlen(password) == 0)
            valid = false;
        if (strcmp(password, confirm) != 0)
        {
            passMismatch = true;
            valid = false;
        }
        if (!IsGmail(email))
        {
            emailError = true;
            valid = false;
        }

        if (valid)
        {
            if (selectedRole == 0)
            {
                otpCursor = 0;
                memset(inputOtp, 0, 5);
                otpError = false;
                showOtpModal = true;
            }
            else
            {
                int res = astra_register(&g_db, fullname, username, email, password, "peminjam");
                if (res == 1)
                {
                    ResetRegisterFields();
                    return STATE_LOGIN;
                }
            }
        }
    }

    // Tombol Kembali
    // Jarak antar tombol 55px sudah cukup
    if (DrawModernBtn(formX, currentY + 55, fieldW, "KEMBALI", COL_BTN_BACK, MAROON, 7))
    {
        ResetRegisterFields();
        return STATE_MENU;
    }

    if (IsKeyPressed(KEY_GRAVE) || IsKeyPressed(KEY_ESCAPE))
    {
        ResetRegisterFields();
        return STATE_MENU;
    }

    return STATE_REGISTER;
}