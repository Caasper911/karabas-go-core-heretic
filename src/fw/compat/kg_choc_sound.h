/* karabas: the part of chocolate-doom 2.3.0 src/i_sound.h that its i_oplmusic.c needs and the
   i_sound.h of doomgeneric lacks (the music_module_t of both is the same). Force-included. */
#pragma once
typedef enum {
    opl_doom1_1_666,    // Doom 1 v1.666
    opl_doom2_1_666,    // Doom 2 v1.666, Hexen, Heretic
    opl_doom_1_9        // Doom v1.9, Strife
} opl_driver_ver_t;
void I_SetOPLDriverVer(opl_driver_ver_t ver);
