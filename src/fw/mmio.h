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
#define MMIO_TICS    MMIO_REG(0x24) /* write: game tic index (marks tics run without a frame) */

#define SRAM_BASE  0x00000000u
#define SDRAM_BASE 0x10000000u
#define WAD_BASE   0x11000000u      /* the WAD, loaded by the sim (on the board: by the FL item) */
#define FB_BASE    (SRAM_BASE + 0x3f0000u) /* 320x200x8 frame in SRAM (below the 4 KB resume area) */
