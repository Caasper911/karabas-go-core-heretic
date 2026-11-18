/* Ф3 (karabas): the files of Doom that live on: default.cfg and the 6 savegames, in the save file
 * that the firmware of the board makes next to the WAD (<WAD dir>/saves/<WAD name>.sav, 1 MB:
 * the backup RAM request of doom_sd.v). The RAM files of syscalls.c stay the working copies:
 * - kg_save_load(name): before a file is opened for reading (it is read into its RAM file),
 * - kg_save_store(name, data, size): when a written file is closed or renamed to its name.
 * Layout, in 512-byte sectors: 0 the table (magic, then the size in bytes of each slot, 0 = no
 * file), slot 0 (default.cfg) from 1, 16 sectors; slot 1 + n (doomsav<n>.dsg) from 17 + 338 n,
 * 338 sectors (173056 bytes: the vanilla limit is 180224, the saves of the IWAD levels are far
 * smaller). Without a save file (no SD card, an old firmware) nothing is kept, as before. */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "mmio.h"
#include "kg_save.h"

#define SD_SECTOR  MMIO_REG(0x80)
#define SD_CMD     MMIO_REG(0x84)
#define SD_STAT    MMIO_REG(0x88)
#define SD_SIZE    MMIO_REG(0x8c)
#define SD_BKREQ   MMIO_REG(0x90)
#define SD_BUF(i)  MMIO_REG(0x1000 + 4 * (i))

#define FILE_SECTORS 2048
#define SLOTS        7
#define CFG_SECTORS  16
#define SAVE_SECTORS 338
static const char MAGIC[8] = "KGDOOMS1";

static int ready;
static uint32_t table[SLOTS];

static int wait_idle(void) {
    uint32_t t0 = MMIO_MS;
    while (SD_STAT & 1) if (MMIO_MS - t0 > 3000) return 0;
    return !(SD_STAT & 2);
}

static int sec_read(uint32_t s, uint8_t *buf) {
    SD_SECTOR = s;
    SD_CMD = 1;
    if (!wait_idle()) return 0;
    for (int i = 0; i < 512; i++) buf[i] = (uint8_t) SD_BUF(i);
    return 1;
}

static int sec_write(uint32_t s, const uint8_t *buf) {
    for (int i = 0; i < 512; i++) SD_BUF(i) = buf[i];
    SD_SECTOR = s;
    SD_CMD = 2;
    return wait_idle();
}

static int table_write(void) {
    uint8_t b[512];
    memset(b, 0, sizeof b);
    memcpy(b, MAGIC, 8);
    memcpy(b + 8, table, sizeof table);
    return sec_write(0, b);
}

/* the slot of a file name (its last path element), -1: not kept */
static int slot_of(const char *name) {
    const char *b = strrchr(name, '/');
    b = b ? b + 1 : name;
    size_t n = strlen(b);
    if (n >= 11 && !strcmp(b + n - 11, "default.cfg")) return 0;
    if (!strncmp(b, "doomsav", 7) && b[7] >= '0' && b[7] <= '5' && !strcmp(b + 8, ".dsg")) return 1 + (b[7] - '0');
    return -1;
}

static uint32_t slot_start(int s) { return s == 0 ? 1 : 1 + CFG_SECTORS + (uint32_t) (s - 1) * SAVE_SECTORS; }
static uint32_t slot_max(int s) { return (s == 0 ? CFG_SECTORS : SAVE_SECTORS) * 512u; }

void kg_save_init(void) {
    ready = 0;
    SD_BKREQ = FILE_SECTORS;
    uint32_t t0 = MMIO_MS;
    while (!(SD_STAT & 4)) if (MMIO_MS - t0 > 2500) { printf("Saves: no save file\n"); return; }
    if (SD_SIZE < FILE_SECTORS * 512u) { printf("Saves: the save file is too small\n"); return; }
    uint8_t b[512];
    if (!sec_read(0, b)) { printf("Saves: read error\n"); return; }
    if (memcmp(b, MAGIC, 8)) {
        memset(table, 0, sizeof table);       /* a new file (zeros): an empty table */
        if (!table_write()) { printf("Saves: write error\n"); return; }
    } else
        memcpy(table, b + 8, sizeof table);
    for (int i = 0; i < SLOTS; i++) if (table[i] > slot_max(i)) table[i] = 0;
    ready = 1;
    printf("Saves: on the SD card\n");
}

int kg_save_kept(const char *name) { return ready && slot_of(name) >= 0; }

/* the size of a kept file, -1: none */
int kg_save_size(const char *name) {
    int s = slot_of(name);
    if (!ready || s < 0 || table[s] == 0) return -1;
    return (int) table[s];
}

int kg_save_load(const char *name, uint8_t *out, uint32_t cap) {
    int s = slot_of(name);
    if (!ready || s < 0 || table[s] == 0 || table[s] > cap) return -1;
    uint32_t size = table[s], start = slot_start(s);
    uint8_t b[512];
    for (uint32_t off = 0; off < size; off += 512) {
        if (!sec_read(start + off / 512, b)) return -1;
        uint32_t n = size - off < 512 ? size - off : 512;
        memcpy(out + off, b, n);
    }
    return (int) size;
}

int kg_save_store(const char *name, const uint8_t *data, uint32_t size) {
    int s = slot_of(name);
    if (!ready || s < 0) return 0;
    if (size > slot_max(s)) { printf("Saves: %s too big (%lu)\n", name, (unsigned long) size); return 0; }
    uint32_t start = slot_start(s);
    uint8_t b[512];
    for (uint32_t off = 0; off < size; off += 512) {
        uint32_t n = size - off < 512 ? size - off : 512;
        memset(b, 0, sizeof b);
        memcpy(b, data + off, n);
        if (!sec_write(start + off / 512, b)) { printf("Saves: write error\n"); return 0; }
    }
    table[s] = size;
    return table_write();
}

void kg_save_remove(const char *name) {
    int s = slot_of(name);
    if (!ready || s < 0 || table[s] == 0) return;
    table[s] = 0;
    table_write();
}
