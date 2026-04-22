# Cycle-Accurate NES Emulator

A high-performance, cycle-accurate Nintendo Entertainment System (NES) emulator written in C with a Python-based frontend. This project utilizes `cppyy` for a high-speed C++/Python bridge, `miniaudio` for low-latency sound, and `pygame` for the display and UI.

## Features
- **Cycle-Accurate CPU:** Implementation of the Ricoh 2A03 (MOS 6502).
- **PPU Rendering:** Accurate pixel processing unit logic.
- **APU Audio:** Integrated audio processing unit with multi-channel support (DMC not implemented fully).
- **Mappers** Mappers 000 and 001 implementation
- **Python UI:** Modern UI features including a native file dialog.
- **High-Performance Bridge:** Uses `cppyy` to call C functions with minimal overhead.

## Requirements
- macOS (Optimized for Apple Silicon / M1)
- Python 3.10+
- requirements.txt
- A C++ compiler (clang/gcc)

## Installation & Usage
Follow these steps to build and run the emulator:

1. **Clone the repository:**
   git clone [https://github.com/regk01/nes-emulator.git](https://github.com/regk01/nes-emulator.git)
   
   cd nes-emulator

3. **Create library**
    make

4. **Setup the Environment**
    make start_env

5. **Run Emulator**
    make run_emulator

## Controls
```text
Key         NES Button / System Function
W           D-Pad Up
S           D-Pad Down
A           D-Pad Left
D           D-Pad Right
Return      Start
Tab         Select
Space       A Button
L-Shift     B Button
P           Toggle Pause
Cmd+Q       Quit Emulator
]           Skip to level 2-2 in Super Mario Bros (Don't press in other games. I like the underwater tune and skip just to hear it)
```

## Architecture
The project follows a modular design separating high-level UI logic from low-level hardware emulation.

    - C/C++ Core (libnes_core.dylib): Manages the time-critical emulation of the 6502 CPU, PPU, and APU. It handles memory mapping, instruction cycles, and generates the raw XRGB pixel buffer.

    - Python Frontend: Built with Pygame, it handles the OS window, event polling, and UI widgets.

    - The Bridge: Utilizes cppyy for dynamic C/C++ bindings, allowing Python to access the C core's memory buffers (like the screen and audio buffers) with zero-copy efficiency.

    - Audio Threading: Uses miniaudio on a dedicated C++ thread to ensure consistent 44.1kHz playback regardless of the Python GIL or UI frame rate.

Debugging & Research
    - This emulator was built as a research project into systems programming. I was curious about how the NES system and thought this was the next stage after creating a CHIP8 Emulator a couple of years ago.

Key research areas included:
    - Synchronization: Managing race conditions between the Python UI thread and the C++ audio/emulation threads.

    - Memory Locality: Optimizing the pixel buffer transfer from the C++ core to pygame.Surface using numpy views for maximum throughput.

    - UI Interaction: Integrating Tkinter into an SDL2/Pygame event loop without thread deadlocks.
