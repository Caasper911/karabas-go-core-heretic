/* Ф0 bench (karabas): doomgeneric platform for the ISS / Verilator bench. No input; the frame
   (CMAP256, 320x200) goes to the SRAM frame buffer; each frame and each game tic is marked in
   MMIO so the sim logs its counters. Args: ARGS from the Makefile (e.g. -timedemo demo1). */
#include <stdint.h>
#include <stdio.h>
#include "doomgeneric.h"
#include "mmio.h"

static uint32_t frame;

void DG_Init(void) { DG_ScreenBuffer = (pixel_t *)FB_BASE; }
void DG_DrawFrame(void) { MMIO_FRAME = frame++; }
void DG_SleepMs(uint32_t ms) { uint32_t t = MMIO_MS; while (MMIO_MS - t < ms) {} }
uint32_t DG_GetTicksMs(void) { return MMIO_MS; }
int DG_GetKey(int *pressed, unsigned char *key) { (void)pressed; (void)key; return 0; }
void DG_SetWindowTitle(const char *title) { (void)title; }

int main(void) {
    /* one writable string, split in place at the spaces */
    static char args[] = "doom -iwad game.wad -mb " MB " " ARGS;
    static char *av[32];
    int n = 0;
    char *s = args;
    while (*s && n < 31) {
        while (*s == ' ') *s++ = 0;
        if (!*s) break;
        av[n++] = s;
        while (*s && *s != ' ') s++;
    }
    av[n] = 0;
    doomgeneric_Create(n, av);
    for (;;) doomgeneric_Tick();
}
