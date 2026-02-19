# Sigmidi: ALSA sequencer based MIDI visualization library 
## 1. Overview

This is a library that uses the ALSA Sequencer API to handle MIDI events and visualize them. You can either use the default RayLib renderer or write your own custom renderer. Inspired from (sightread)[sightread.dev/freeplay]
rem to add image.

## 2. Dependencies
1. gcc
2. libasound
3. libm
4. raylib (or your preferred graphics library)

NOTE: Only compatible with Linux.

## 3. Compilation
```bash
make
./build/main.out "<alsa client name>:<port>"
```

## 4. Install
```bash
make install
sigmidi "<alsa client name>:<port>"
```

## 3. Implementing a Custom Renderer

Just write your own implementations for the functions defined in `include/sigmidi-renderer.h`. Note the the library owns the event loop and you just provide the implementations.

## 5. Player keybindings

| Key                 | Action                                            |
| ------------------- | ------------------------------------------------- |
| **Up / Down**       | Increase/Decrease Octave Count                    |
| **Shift + (+/-)**   | Shift Octave Offset (Transpose View)              |
| **Shift + (< / >)** | Adjust BPM (Tempo)                                |
| **V**               | Toggle Velocity-Based Coloring (default ON)       |
| **F**               | Toggle Fullscreen                                 |
| **P**               | Toggle Sustain View (default OFF)                 |
| **L (Hold)**        | Show ALSA Client List (Press 1-9 to Subscribe)    |
| **S (Hold)**        | Show Subscription List (Press 1-9 to Unsubscribe) |

