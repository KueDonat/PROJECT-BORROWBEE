#include <stdio.h>
#include "db_astra.h"
#include <raylib.h>
#include "common.h"

// --- JANGAN ADA #include "dashboard_admin.c" di sini jika pakai perintah gcc di bawah ---

AstraDB g_db;
int g_currentUserId = 0;
char g_currentUserRole[20] = "";

int main(void)
{
    InitWindow(1320, 780, "BorrowBee System");

    SetTargetFPS(60);

    if (!astra_connect(&g_db, "project_astra")) // Ganti DSN sesuai ODBC kamu
    {
        printf("Gagal Connect DB!\n");
    }

    Texture2D imageLogo = LoadTexture("Borrowbee.png");

    // Inisialisasi awal dashboard admin (opsional, biar tabel siap)
    void InitDashboardAdmin(AstraDB * db);

    AppState currentScreen = STATE_MENU;

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (IsKeyPressed(KEY_F11))
        {
            ToggleFullscreen();
        }

        switch (currentScreen)
        {
        case STATE_MENU:
            // Panggil fungsi dari menu.c
            currentScreen = DrawMenuScreen(imageLogo);
            break;

        case STATE_REGISTER:
            // Panggil fungsi dari register.c
            currentScreen = DrawRegisterScreen(imageLogo);
            break;

        case STATE_LOGIN:
            // Panggil fungsi dari login.c
            // Pastikan login.c mengembalikan STATE_DASHBOARD_ADMIN atau STATE_DASHBOARD_USER jika sukses
            currentScreen = DrawLoginScreen(imageLogo);
            break;

        case STATE_DASHBOARD_ADMIN:
            // Panggil fungsi dari dashboard_admin.c
            currentScreen = DrawDashboardAdmin(&g_db, imageLogo);
            break;

        case STATE_DASHBOARD_USER:
            // Panggil fungsi dari dashboard_user.c
            currentScreen = DrawDashboardUser(&g_db, imageLogo);
            break;

        case STATE_EXIT:
            goto end_app;
        }

        EndDrawing();
    }

end_app:
    UnloadTexture(imageLogo);
    astra_disconnect(&g_db);
    CloseWindow();
    return 0;
}
