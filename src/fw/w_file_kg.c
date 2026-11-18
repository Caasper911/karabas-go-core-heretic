/* Ф0 bench (karabas): the WAD is already in memory at WAD_BASE (on the board: loaded by the FL
   item), so the file is "mapped": W_CacheLumpNum returns pointers into it, no copies in the zone.
   Replaces upstream w_file_stdc.c (same class name). */
#include <stdio.h>
#include <string.h>
#include "w_file.h"
#include "z_zone.h"
#include "mmio.h"

int kg_is_wad(const char *name);
extern wad_file_class_t stdc_wad_file;

static wad_file_t *W_Kg_OpenFile(char *path) {
    if (!kg_is_wad(path)) return NULL;
    wad_file_t *w = Z_Malloc(sizeof(wad_file_t), PU_STATIC, 0);
    w->file_class = &stdc_wad_file;
    w->mapped = (byte *)WAD_BASE;
    w->length = MMIO_WADSIZE;
    return w;
}

static void W_Kg_CloseFile(wad_file_t *w) { Z_Free(w); }

static size_t W_Kg_Read(wad_file_t *w, unsigned int offset, void *buffer, size_t len) {
    if (offset >= w->length) return 0;
    if (len > w->length - offset) len = w->length - offset;
    memcpy(buffer, w->mapped + offset, len);
    return len;
}

wad_file_class_t stdc_wad_file = { W_Kg_OpenFile, W_Kg_CloseFile, W_Kg_Read };
