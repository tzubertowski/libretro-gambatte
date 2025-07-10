# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Build for sf2000 platform (MIPS32 architecture)
make platform=sf2000 -j12

# Build the libretro core for unix/linux (parallel compilation recommended)
make -j4

# Clean build artifacts
make clean

# Build for specific platform (example: Android)
make platform=android

# Build with debug symbols
make DEBUG=1
```

## Architecture Overview

This is **libretro-gambatte**, an accuracy-focused Game Boy/Game Boy Color emulator core for the libretro framework. The codebase follows a layered architecture:

### Core Emulation Engine (`libgambatte/src/`)
- **CPU** (`cpu.h/cpp`) - Main execution loop and Game Boy CPU emulation
- **Memory** (`gambatte-memory.h/cpp`) - Memory mapping and management
- **Video** (`video.h/cpp`) - PPU emulation with accurate timing
- **Sound** (`sound.h/cpp`) - 4-channel audio synthesis

### Memory Subsystem (`libgambatte/src/mem/`)
- **Cartridge** (`cartridge.h/cpp`) - ROM/RAM handling with MBC support
- **RTC** (`rtc.h/cpp`) - Real-time clock for supported cartridges
- MBC1, MBC3, and HuC3 memory bank controller implementations

### Audio Subsystem (`libgambatte/src/sound/`)
- Four Game Boy sound channels with accurate emulation
- Custom resampling using blipper for high-quality output
- Individual channel implementations (channel1-4.h/cpp)

### Video Subsystem (`libgambatte/src/video/`)
- **PPU** (`ppu.h/cpp`) - Picture Processing Unit emulation
- **Sprite Mapper** (`sprite_mapper.h/cpp`) - OAM and sprite handling
- **LY Counter** (`ly_counter.h/cpp`) - LCD line timing

### Libretro Interface (`libgambatte/libretro/`)
- **libretro.cpp** - Main libretro API implementation
- **Core options** extensive configuration system
- **Network support** for Game Link Cable multiplayer
- **Save state** and debugging interfaces

## Key Development Notes

### Code Standards
- **C++98 compatibility** required for broad platform support
- **No exceptions or RTTI** (`-fno-exceptions -fno-rtti`)
- Platform-specific optimizations handled in Makefile.libretro

### Important Interfaces
- `gambatte::GB` class in `libgambatte/include/gambatte.h` is the main emulator API
- `retro_*` functions in `libgambatte/libretro/libretro.cpp` handle libretro integration
- Core options defined in `libretro_core_options.h` control runtime behavior

### Timing and Accuracy
This emulator prioritizes accuracy over performance. When modifying timing-critical code:
- CPU execution is cycle-accurate
- PPU timing matches hardware behavior
- Audio synthesis maintains proper sample rates
- Memory access timing affects emulation accuracy

### Build System
The Makefile.libretro supports extensive platform targets. When adding features:
- Check platform compatibility flags
- Test across multiple architectures if possible
- Consider performance impact on resource-constrained platforms

### Testing and Validation
- No formal test suite exists - testing is done through ROM compatibility
- The emulator has been validated against hundreds of corner case hardware tests
- When making changes, test with known problematic ROMs and homebrew test ROMs
- Save states and debugging features are available for testing and verification

### Common File Locations
- **Main API**: `libgambatte/include/gambatte.h` - Primary interface for GB emulation
- **Libretro Interface**: `libgambatte/libretro/libretro.cpp` - RetroArch integration
- **Core Options**: `libgambatte/libretro/libretro_core_options.h` - Runtime configuration
- **Network Support**: `libgambatte/libretro/net_serial.cpp` - Game Link Cable emulation
- **Palette Data**: `libgambatte/libretro/gbcpalettes.h` - Color correction and GBC palettes
- **Audio Resampling**: `libgambatte/libretro/blipper.h` - Custom audio resampling
- **Build Configuration**: `Makefile.libretro` - Platform-specific build settings