# karabas-go-core-doom

Doom for the Karabas Go, Mini and Mini G boards: a VexRiscv soft CPU running [doomgeneric](https://github.com/ozkl/doomgeneric), with the frame buffer, palette, sound mixer, OPL3 FM music and MIDI (SAM2695) in the FPGA. Work in progress: plan and measurements in `docs/PLAN.md` (Russian).

The core contains no game data: load your own IWAD (DOOM.WAD, DOOM2.WAD, ... or Freedoom) from the SD card.

## Licence

GPL-3.0 (`LICENSE`), as the other karabas-go cores. Imported parts keep their own licences: the Doom sources (doomgeneric / Chocolate Doom) are GPL-2.0-or-later, VexRiscv is MIT, opl3_fpga is LGPL-3.0.
