# WinBlocks

**WINBLOCKS!** is a WIP Windows recreation of **TETRIS** for the Nintendo GameBoy, built from scratch in **C++ with SDL2**!

My goal is to recreate the original feel, style, controls and sound of GameBoy **TETRIS** without the use of an emulator, **WINBLOCKS IS NOT A GAMEBOY EMULATOR**!

# ROM REQUIRED!!
WinBlocks requires an **UNMODIFIED** version of **TETRIS** to read artwork, layouts, music and sound data from! 
**NO ROM WILL EVER BE PROVIDED AND NO ORIGINAL GAME ASSETS ARE INCLUDED IN THIS REPOSITORY!**

## FEATURES
- A 100% accurate and playable **SINGLE PLAYER A-TYPE** mode
- Level Selection and Gravity speeds, Soft Drops and Scoring
- the Original Layouts and Menus
- Fullscreen and Widescreen with aspect-correct GameBoy rendering
- Custom four-channel Game Boy-inspired audio synthesis
- Original ROM music options
- Local High Score table (saved in "winblocks_scores.txt")
- Discord Rich Presence

## COMING SOON
- B-TYPE mode!
- Modern C-TYPE mode with Hard Drops and Ghost Pieces
- Colour Selector with prefrence saving
- Optional  QoL features
- Setting Menu with volume sliders!

## CONTROLS (KEYBOARD)
Movement - Arrow Keys
Rotate Piece - Z/X/Space 
Soft Drop - Down Arrow
Start - Enter
Pause - also the Enter key
Scoreboard - X to confirm letter, Z to go back and UP/DOWN to swap letters
Hide Next Piece - Backspace
Disable Music In Game - F6
Fullscreen - F11

## CONTROLS (CONTROLLER)
Movement - Dpad
Rotate Piece - A/B 
Soft Drop - Dpad Down
Start - Start Button
Pause - also the Start Button
Scoreboard - A to confirm letter, B to go back and Dpad UP/DOWN to swap letters
Hide Next Piece - Currently Not Implimented
Disable Music In Game - Currently Not Implimented
Fullscreen - View Button (that small one on the left side of the XBOX controller)

## Build Requirements
- Windows and Visual Studio with the MSVC C++ toolchain (C++20 for the Discord Social SDK integration)
- **SDL2** headers, import libraries, and matching runtime DLL
- **Discord Social SDK** headers, import library, and matching runtime DLL for the Rich Presence integration
- A **legally obtained** compatible original GameBoy **TETRIS** ROM, named `tetris.gb`

The SDL2 and Discord SDK dependencies are **not committed** to this repository. Configure the solution's include paths, library paths and linker inputs to point to your local SDK installations. The source currently includes `SDL.h` and `discordpp.h` and links against the SDKs

## HOW TO BUILD AND RUN
1. Open the included Visual Studio `.sln` file
2. Configure the SDL2 and Discord Social SDK include/library paths in **Project → Properties**, targeting the platform for which you have the SDK libraries
3. Use **C++20** in **C/C++ → Language**
4. Build the project and ensure the required SDL2 and Discord SDK DLLs are available beside the built executable
5. Place your own compatible `tetris.gb` in the program's **working directory**, the source opens it by that relative filename. The working directory depends on the Visual Studio run configuration
6. Run WinBlocks

The ROM must pass the cartridge title and checksum checks in the loader. This repository does not provide a way to acquire copyrighted game data.

## Disclaimer
WinBlocks is an unofficial fan-made recreation and is not affiliated with or endorsed by Nintendo or the Tetris Company. Third-party trademarks, game artwork, original ROM content, and SDKs remain the property of their respective owners. Please dont send the ninjas to my house.

## Disclaimer Part Two
Some AI assistance was used in this project for general assistance and to help me understand parts of code.