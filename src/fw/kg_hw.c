/* Ф1 (karabas): the doomgeneric platform of the board (src/rtl/doom_soc.v MMIO).
 * - Waits for the WAD (the FL item "WAD", MMIO_WADSIZE != 0) with a test picture (all 256 colours
 *   of a ramp palette + the log), then starts Doom.
 * - Keys: the USB boot report of kg_mcu2 (KBD0 / KBD1, KBDN counts reports) compared with the last
 *   one -> Doom key events (USB usage -> doomkeys.h). Mouse: the sums of MMIO_MOUSE -> ev_mouse.
 *   Pad (JOY, port 0): arrows, A = fire, B = use, C = run (shift), start = Enter, X = Esc.
 * - The console (printf) goes to MMIO_UART and to a log of the last lines; at exit (I_Error, quit)
 *   the log is drawn on the screen in the 8x8 font (font8.h) with a grey palette, and the CPU waits. */
#include <stdint.h>
#include <string.h>
#include "doomgeneric.h"
#include "doomkeys.h"
#include "d_event.h"
#include "mmio.h"
#include "font8.h"
#include "kg_irq.h"
#include "kg_save.h"

#define MMIO_KBD0  MMIO_REG(0x28)   /* key3 key2 key1 modifiers */
#define MMIO_KBD1  MMIO_REG(0x2c)   /* 0 key6 key5 key4 */
#define MMIO_KBDN  MMIO_REG(0x30)
#define MMIO_MOUSE MMIO_REG(0x34)   /* buttons[31:24], dy[23:12], dx[11:0] */
#define MMIO_JOY   MMIO_REG(0x38)

/* ------------------------------------------------------------------ the log */

#define LOG_LINES 25
#define LOG_COLS 40
static char logbuf[LOG_LINES][LOG_COLS + 1];
static int log_line, log_col;

void kg_log_putc(char c) {
    if (c == '\n' || log_col == LOG_COLS) {
        log_line = (log_line + 1) % LOG_LINES;
        memset(logbuf[log_line], 0, LOG_COLS + 1);
        log_col = 0;
        if (c == '\n') return;
    }
    if (c >= 32 && c < 127) logbuf[log_line][log_col++] = c;
}

static void draw_char(uint8_t *fb, int x, int y, char c, uint8_t col) {
    const unsigned char *g = font8[(c >= 32 && c < 127) ? c - 32 : 0];
    for (int r = 0; r < 8; r++)
        for (int b = 0; b < 8; b++)
            if (g[r] & (0x80 >> b)) fb[(y + r) * 320 + x + b] = col;
}

static void draw_log(uint8_t *fb, uint8_t col) {
    for (int i = 0; i < LOG_LINES; i++) {
        const char *s = logbuf[(log_line + 1 + i) % LOG_LINES];
        for (int k = 0; s[k]; k++) draw_char(fb, k * 8, i * 8, s[k], col);
    }
}

static void grey_palette(void) {
    for (int i = 0; i < 256; i++) MMIO_PAL(i) = (i << 16) | (i << 8) | i;
}

/* the end: the log on the screen (called by _exit) */
void kg_exit_screen(int code) {
    uint8_t *fb = (uint8_t *)FB_BASE;
    grey_palette();
    memset(fb, code ? 0x30 : 0x00, 64000);
    draw_log(fb, 0xff);
    MMIO_FRAME = 0;
}

/* ------------------------------------------------------------------ keys */

static unsigned char usb_to_doom(uint8_t u) {
    if (u >= 0x04 && u <= 0x1d) return 'a' + (u - 0x04);
    if (u >= 0x1e && u <= 0x26) return '1' + (u - 0x1e);
    if (u >= 0x3a && u <= 0x43) return KEY_F1 + (u - 0x3a);
    switch (u) {
    case 0x27: return '0';
    case 0x28: case 0x58: return KEY_ENTER;
    case 0x29: return KEY_ESCAPE;
    case 0x2a: return KEY_BACKSPACE;
    case 0x2b: return KEY_TAB;
    case 0x2c: return KEY_USE;
    case 0x2d: return KEY_MINUS;
    case 0x2e: return KEY_EQUALS;
    case 0x2f: return '[';
    case 0x30: return ']';
    case 0x31: return '\\';
    case 0x33: return ';';
    case 0x34: return '\'';
    case 0x35: return '`';
    case 0x36: return ',';
    case 0x37: return '.';
    case 0x38: return '/';
    case 0x39: return KEY_CAPSLOCK;
    case 0x44: return KEY_F11;
    case 0x45: return KEY_F12;
    case 0x48: return KEY_PAUSE;
    case 0x49: return KEY_INS;
    case 0x4a: return KEY_HOME;
    case 0x4b: return KEY_PGUP;
    case 0x4c: return KEY_DEL;
    case 0x4d: return KEY_END;
    case 0x4e: return KEY_PGDN;
    case 0x4f: return KEY_RIGHTARROW;
    case 0x50: return KEY_LEFTARROW;
    case 0x51: return KEY_DOWNARROW;
    case 0x52: return KEY_UPARROW;
    case 0x56: return KEY_MINUS;     /* keypad - / + : the view size */
    case 0x57: return KEY_EQUALS;
    }
    return 0;
}

static const unsigned char mod_key[8] = { KEY_FIRE, KEY_RSHIFT, KEY_RALT, 0, KEY_FIRE, KEY_RSHIFT, KEY_RALT, 0 };

#define QN 64
static uint16_t q[QN];
static unsigned qr, qw;
static void push(int pressed, unsigned char k) {
    if (!k || ((qw + 1) % QN) == qr) return;
    q[qw] = (pressed << 8) | k;
    qw = (qw + 1) % QN;
}

static uint8_t last_rep[7];
static uint32_t last_kbdn;
static uint32_t last_joy;

static int has(const uint8_t *r, uint8_t u) {
    for (int i = 1; i < 7; i++) if (r[i] == u) return 1;
    return 0;
}

static void poll_input(void) {
    uint32_t n = MMIO_KBDN;
    if (n != last_kbdn) {
        last_kbdn = n;
        uint32_t k0 = MMIO_KBD0, k1 = MMIO_KBD1;
        uint8_t rep[7] = { k0 & 0xff, (k0 >> 8) & 0xff, (k0 >> 16) & 0xff, k0 >> 24, k1 & 0xff, (k1 >> 8) & 0xff, (k1 >> 16) & 0xff };
        for (int b = 0; b < 8; b++) {
            int was = (last_rep[0] >> b) & 1, now = (rep[0] >> b) & 1;
            if (was != now) push(now, mod_key[b]);
        }
        for (int i = 1; i < 7; i++) {
            if (last_rep[i] > 3 && !has(rep, last_rep[i])) push(0, usb_to_doom(last_rep[i]));
            if (rep[i] > 3 && !has(last_rep, rep[i])) push(1, usb_to_doom(rep[i]));
        }
        memcpy(last_rep, rep, 7);
    }
    uint32_t j = MMIO_JOY & 0xffff;
    if (j != last_joy) {
        static const unsigned char jk[10] = { 0, KEY_UPARROW, KEY_DOWNARROW, KEY_LEFTARROW, KEY_RIGHTARROW,
                                              KEY_ENTER, KEY_FIRE, KEY_USE, KEY_RSHIFT, KEY_ESCAPE };
        for (int b = 1; b < 10; b++)
            if (((j ^ last_joy) >> b) & 1) push((j >> b) & 1, jk[b]);
        last_joy = j;
    }
    uint32_t m = MMIO_MOUSE;
    int dx = (int32_t)(m << 20) >> 20, dy = (int32_t)(m << 8) >> 20, mb = m >> 24;
    static int last_mb;
    if (dx || dy || mb != last_mb) {
        event_t ev;
        ev.type = ev_mouse;
        ev.data1 = mb & 7;
        ev.data2 = dx * 4;
        ev.data3 = -dy * 4;
        D_PostEvent(&ev);
        last_mb = mb;
    }
}

/* ------------------------------------------------------------------ doomgeneric */

static uint32_t frame;

void DG_Init(void) {}
void DG_DrawFrame(void) { MMIO_FRAME = frame++; }
void DG_SleepMs(uint32_t ms) { uint32_t t = MMIO_MS; while (MMIO_MS - t < ms) {} }
uint32_t DG_GetTicksMs(void) { return MMIO_MS; }
void DG_SetWindowTitle(const char *title) { (void)title; }
int DG_GetKey(int *pressed, unsigned char *key) {
    if (qr == qw) poll_input();
    if (qr == qw) return 0;
    *pressed = q[qr] >> 8;
    *key = q[qr] & 0xff;
    qr = (qr + 1) % QN;
    return 1;
}

int printf(const char *, ...);

static void wait_wad(void) {
    uint8_t *fb = (uint8_t *)FB_BASE;
    for (int i = 0; i < 256; i++) MMIO_PAL(i) = ((i & 0xe0) << 16) | ((i & 0x1c) << 11) | ((i & 0x03) << 6);
    for (int y = 0; y < 200; y++)
        for (int x = 0; x < 320; x++) fb[y * 320 + x] = (uint8_t)((y / 25) * 32 + x / 10);
    printf("Karabas Doom: load a WAD (Menu+F1)\n");
    for (int i = 0; i < 4; i++) draw_char(fb, 8 * i, 192, "WAD?"[i], 0xff);
    draw_log(fb, 0xff);
    MMIO_FRAME = 0;
    while (MMIO_WADSIZE == 0) {}
}

int main(void) {
#ifndef HWARGS
#define HWARGS ""
#endif
    static char args[] = "doom -iwad game.wad -mb 8 " HWARGS;
    static char *av[16];
    int n = 0;
    char *s = args;
    while (*s && n < 15) {
        while (*s == ' ') *s++ = 0;
        if (!*s) break;
        av[n++] = s;
        while (*s && *s != ' ') s++;
    }
    av[n] = 0;
    kg_irq_init();
    wait_wad();
    kg_save_init();
    doomgeneric_Create(n, av);
    for (;;) doomgeneric_Tick();
}
