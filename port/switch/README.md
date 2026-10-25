# ReKHCoM for Nintendo Switch (alpha)

**Alpha, never run on a console yet.** A homebrew `.nro` for a Switch running custom firmware (Atmosphère), started
from the Homebrew Menu in title takeover mode (hold R while opening a game), not from the album applet.

## How it works

The game needs 32-bit pointers, and the Switch only runs 64-bit programs that cannot map the GBA's low addresses.

- The game and the port layer are one image of AArch64 code with 32-bit pointers (clang's `arm64_32` target, Mach-O
  objects linked at `0x10000000`, built for the Cortex-A57), served through the PS5 bridge
  (`port/ps5/shared/bridge_list.h`).
- The GBA space is relocatable (`PORT_GBA_RELOC`, `include/gba/gba_space.h`): every region is an array of the image.
  When the ROM is loaded, its pointer words (`port/data/rom_pointers_*.inc`, from `port/tools/gen_rom_pointers.py`)
  are moved into those arrays.
- `host/switch_host.c` (libnx) maps the image into the process's code region, draws the game's quads with a software
  rasterizer into a 1280x720 SDL2 texture, plays 48 kHz sound through SDL2 and reads the controllers through libnx.

Without `__SWITCH__` the same file is a headless Linux AArch64 test host: the ARM64 VM runs the image with the
game's harness variables (`KHCOM_BATTLE`, `KHCOM_KEYS`, ...) and `--shot N file.ppm`.

## Install

1. Copy `ReKHCoM.nro` to `sdmc:/switch/ReKHCoM/`.
2. Copy your own cartridge dump (EU, US or JP, any name ending in `.gba`) to the same folder.
3. Start ReKHCoM from the Homebrew Menu (title takeover). The log is `sdmc:/switch/ReKHCoM/logs/ReKHCoM.log`;
   saves and settings sit next to the ROM.

Controls: Nintendo layout (A confirms), L/R, ZL/ZR, D-pad or left stick, Plus = Start, Minus = Select;
Minus + L3 saves a state, Minus + R3 loads it.

## Build

```
python port/switch/build_switch.py guest          # build/switch/guest-<region>.macho (Windows or Linux)
python port/switch/build_switch.py host           # build/switch/ReKHCoM.nro
```

The host is built in the consoles VM (`port/progress/TOOLKIT_CONSOLAS.md`) with devkitA64 and libnx in
`/opt/devkitpro`, taken from the `devkitpro/devkita64` Docker image (devkitPro's own servers refuse the VM).

## HD mod (optional)

The HD mod (Re:Chain of Memories' music, HD cards/HUD and subtitles) is made on a PC from your own
KINGDOM HEARTS -HD 1.5+2.5 ReMIX-:

1. On the PC, open ReKHCoM (the Windows or Linux download of this same release) > Mod HD and load it from the game.
2. Under "Take the HD mod to another platform" choose **Nintendo Switch** and press "Make ReKHCoM-HD.zip".
3. Copy that one file, as it is, to `sdmc:/switch/ReKHCoM/` and start ReKHCoM: it installs itself (the log says
   `hd pack: N files ...`). You may delete the zip afterwards.

A `hd` folder copied there by hand works too. HD movies do not play on the Switch yet; the music and cards do.
