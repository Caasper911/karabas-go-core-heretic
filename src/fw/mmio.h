/* Ф0 bench MMIO (src/sim/iss.cpp). Memory map: SRAM 4 MB at 0x00000000, SDRAM 32 MB at 0x10000000,
   MMIO at 0x80000000 (VexRiscv IO range = bit 31, uncached). */
#pragma once
#include <stdint.h>

#define MMIO_REG(o) (*(volatile uint32_t *)(0x80000000u + (o)))
#define MMIO_UART    MMIO_REG(0x00) /* write: console byte */
#define MMIO_FRAME   MMIO_REG(0x04) /* write: frame index, the sim logs the counters of the frame */
#define MMIO_EXIT    MMIO_REG(0x08) /* write: stop the simulation with this code */
#define MMIO_CYCLES  MMIO_REG(0x0c) /* read: clock cycles (ISS: instructions), low 32 bits */
#define MMIO_MS      MMIO_REG(0x1c) /* read: milliseconds (ISS: instructions / 50000) */
#define MMIO_WADSIZE MMIO_REG(0x20) /* read: size of the WAD in memory at WAD_BASE */
#define MMIO_PAL(i)  MMIO_REG(0x400 + 4 * (i)) /* write: palette entry i = 0x00RRGGBB (256 entries) */

#define SRAM_BASE  0x00000000u
#define SDRAM_BASE 0x10000000u
#define WAD_BASE   0x10c00000u      /* the WAD (up to 20 MB), loaded by the sim (on the board: by the FL item) */
#define FB_BASE    (SRAM_BASE + 0x3f0000u) /* 320x200x8 frame in SRAM (below the 4 KB resume area) */

/* Ф2: timer, switches, sound (doom_soc.v, doom_audio.v) */
#define MMIO_US       MMIO_REG(0x40)  /* read: microseconds since reset */
#define MMIO_TCMP     MMIO_REG(0x44)  /* write: the timer interrupt when US - TCMP >= 0 (and TEN) */
#define MMIO_TEN      MMIO_REG(0x48)  /* write: bit 0 enables the timer interrupt */
#define MMIO_SW       MMIO_REG(0x50)  /* read: OSD switches; bits 15..8 = Music (0 OPL3, 1 MIDI, 2 off) */
#define MMIO_OPL_ADDR MMIO_REG(0x60)  /* write: { bank, register } */
#define MMIO_OPL_DATA MMIO_REG(0x64)  /* write: the value */
#define MMIO_MIDI     MMIO_REG(0x68)  /* write: a MIDI byte */
#define MMIO_SFX_POS  MMIO_REG(0x70)  /* read: the ring index played now */
#define MMIO_SFX(i)   MMIO_REG(0x2000 + 4 * (i)) /* write: ring entry i = { right16, left16 } */
#define SFX_RING      2048
#define SFX_RATE      11025
