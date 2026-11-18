/* Ф3 (karabas): default.cfg and the savegames in the save file on the SD card (kg_save.c) */
#pragma once
#include <stdint.h>

void kg_save_init(void);                     /* ask for the save file, read its table */
int kg_save_kept(const char *name);          /* 1: the name is one of the kept files */
int kg_save_size(const char *name);          /* its size, -1: not in the save file */
int kg_save_load(const char *name, uint8_t *out, uint32_t cap);
int kg_save_store(const char *name, const uint8_t *data, uint32_t size);
void kg_save_remove(const char *name);
