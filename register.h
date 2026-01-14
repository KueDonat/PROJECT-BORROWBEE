#ifndef REGISTER_H
#define REGISTER_H

#include <raylib.h>

typedef struct
{
    char username[64];
    char email[64];
    char password[64];
    char confirmPass[64];
    int letterCount[4];
} RegisterData;

// Fungsi-fungsi yang bisa dipanggil dari main.c
void InitRegisterScreen(void);
void UpdateRegisterScreen(void);                                                          // Logika ketikan & navigasi
void DrawRegisterScreen(Texture2D imgWarehouse, Texture2D imgLogo, Rectangle screenRect); // Gambar tampilan

// Helper untuk reset data (opsional)
void ResetRegisterData(void);

#endif