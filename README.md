# Doom core for Karabas Go / Mini / Mini G

[Русская версия](README.ru.md)

Doom for the Karabas Go board family (Xilinx Spartan-6 XC6SLX25): a VexRiscv RISC-V soft CPU at 75 MHz running [doomgeneric](https://github.com/ozkl/doomgeneric) (Chocolate Doom), with the frame buffer, palette, sound mixer, OPL3 FM music and the MIDI synth of the board in the FPGA.

| Board | Core file | Video | Sound |
|---|---|---|---|
| Karabas Go | `doom.kg` | VGA | I2S DAC (PCM5102); the MIDI synth on the analog output only |
| Karabas Mini | `doom.kg` | HDMI | HDMI, analog output (sigma-delta) |
| Karabas Mini G | `doom.kg` | HDMI | HDMI, I2S DAC |

The core file is in `src/`. It contains no game data: put your own IWAD (`DOOM.WAD`, `DOOM2.WAD`, `PLUTONIA.WAD`, `TNT.WAD`, the shareware `DOOM1.WAD` or [Freedoom](https://freedoom.github.io/)) on the SD card.

Status: checked on Mini G (DOOM II: play, music, effects, saves). The Go and Mini builds have not been run on a board yet.

## Features

- **Speed:** about 20-30 frames per second in play (Doom runs its game at 35 tics per second).
- **Video:** 320 x 200 in 256 colours, doubled to 640 x 400 in the middle of 640 x 480 at 60 Hz.
- **Sound effects:** 8 channels mixed by the CPU at 11025 Hz.
- **Music:** OPL3 FM (the GENMIDI instruments of the WAD, as Chocolate Doom) or the MIDI synth of the board (SAM2695), chosen in the OSD.
- **Saves:** the 6 savegames and the settings (`default.cfg`) are kept in `<WAD dir>/saves/<WAD name>.sav` (1 MB, made by the firmware; needs firmware 26111813 or newer).
- **Controls:** the USB keyboard, the mouse, a USB pad.

Not done: PWADs (only one IWAD is loaded), network play.

## Getting started

1. Copy `doom.kg` (one file for Go, Mini and Mini G) to the root of the SD card.
2. Put your WAD in `/doom` on the SD card.
3. Choose Doom in the core menu. The first time open the OSD (**Menu+Esc**) and choose the WAD (**Menu+F1**). Loading a WAD takes a while (DOOM2.WAD, 15 MB: about 25 s); the core loads the last WAD by itself at the next start.

"Menu" is the Windows key of the keyboard.

## OSD menu

| Item | Values | Hotkey |
|---|---|---|
| WAD | `.wad` file (loaded at the core start) | Menu+F1 |
| Music | MIDI / OPL3 / Off | Menu+M |
| Controls | Classic / WASD (see below) | |
| Mouse move | Off / On: the mouse also moves forward and back (as vanilla Doom; Off = only turns) | |
| Reset | restarts Doom | F12 |

## Controls

| Action | Keyboard | Pad |
|---|---|---|
| Move, turn | arrows (WASD: also W / S, A / D strafe) | D-pad |
| Fire | Ctrl, left mouse button | A |
| Use (doors, switches) | Space (WASD: also E, right mouse button) | B |
| Run | Shift | C |
| Strafe | Alt (with the arrows), `,` / `.` | |
| Weapons | 1 .. 7, `[` / `]` previous / next | Z / Y previous / next |
| Menu, Enter | Esc, Enter | X, Start |
| Map | Tab | Mode |
| Save / load | F2 / F3 | |

Mouse buttons in Classic: left fire, right strafe, middle forward. Both pads work (the same buttons). In WASD the letters W, A, S, D, E act as game keys outside the menu only (the savegame names are typed as usual); cheat codes with them do not work in this mode.

The other keys are those of Doom (F1 help, F5 detail, F6 quicksave, F9 quickload, F11 gamma, `-` / `=` screen size).

## Building

The firmware of the CPU is in `src/fw` (xPack riscv-none-elf-gcc 15.2); the gateware is built with ISE 14.7 (`src/Makefile`: `make rom`, then `make minig` / `make mini` / `make go`). The design, the measurements and the history are in `docs/PLAN.md` (Russian).

## Licence

GPL-3.0 (`LICENSE`), as the other karabas-go cores. Imported parts keep their own licences: doomgeneric and Chocolate Doom are GPL-2.0-or-later, VexRiscv is MIT, opl3_fpga is LGPL-3.0.
