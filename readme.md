# WinBlocks

**WINBLOCKS!** is a Windows recreation of **TETRIS** for the Nintendo GameBoy, built from scratch in **C++ with SDL2**!

![WinBlocks](screenshots/Title.png)

My goal is to recreate the original feel, style, controls and sound of GameBoy **TETRIS** without the use of an emulator, **WINBLOCKS IS NOT A GAMEBOY EMULATOR**!

# ROM REQUIRED!!
WinBlocks requires an **UNMODIFIED** version of **TETRIS** to read artwork, layouts, music and sound data from! 
**NO ROM WILL EVER BE PROVIDED AND NO ORIGINAL GAME ASSETS ARE INCLUDED IN THIS REPOSITORY!**

# VERSION 1.1 is **HERE** and INTRODUCING ALL NEW FEATURES!!

![WinBlocks](screenshots/Extras1.png)

- An **EXTRAS** Menu
- Color Selection with 20 different palettes to choose from! **(you can find images of each one at the bottom of this readme)**
- Quick Color Swapping via F3
- An ingame **fullscreen** toggle
- **Hard Drops, Ghost Piece** and **Piece Holding** via the Extras Menu
- Audio Sliders for **music, sounds** and **Master Volume**
- Persistent settings!
- Updated the Score Board
- Extra random additions and fixes

![WinBlocks](screenshots/GameUpdate2.png)

## FEATURES
- A 100% accurate and playable **SINGLE PLAYER A-TYPE** mode
- Level Selection and Gravity speeds, Soft Drops and Scoring
- the Original Layouts and Menus
- Fullscreen and Widescreen with aspect-correct GameBoy rendering
- Custom four-channel Game Boy-inspired audio synthesis
- Original ROM music options
- Local High Score table (saved in "winblocks_scores.txt")
- Discord Rich Presence
- Color Selector with Preference saving
- Optional  QoL features

![WinBlocks](screenshots/Game.png)

## COMING SOON
- B-TYPE mode!
- Modern C-TYPE mode

## CONTROLS (KEYBOARD)
- Movement - Arrow Keys
- Rotate Piece - Z/X/Space 
- Soft Drop - Down
- Hard Drop - Up
- Hold Piece - C
- Start - Enter
- Pause - also the Enter key
- Back - ESC
- Scoreboard - X to confirm letter, Z to go back and UP/DOWN to swap letters
- Hide Next Piece - Backspace
- Disable Music In Game - F6
- Fullscreen - F11
- Settings Menu - F2
- Instant Color Swapping - F3 (F3 + SHIFT to go back)
- EXTRAS Page 1/2 swapping - TAB

![WinBlocks](screenshots/Level.png)

## CONTROLS (CONTROLLER)
- Movement - Dpad
- Rotate Piece - A/B 
- Soft Drop - Down
- Hard Drop - Up
- Hold Piece - RB
- Start - Start Button
- Pause - also the Start Button
- Back - B
- Scoreboard - A to confirm letter, B to go back and Dpad UP/DOWN to swap letters
- Hide Next Piece - Right Stick Press (R3)
- Disable Music In Game - Left Stick Press (L3)
- Fullscreen - View Button (that small one on the left side of the XBOX controller)
- Settings Menu - Y 
- Instant Color Swapping - LT/RT
- EXTRAS Page 1/2 swapping - LB/RB

![WinBlocks](screenshots/TypeUpdate.png)

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

## COLOR PALETTES (in order)

![WinBlocks](screenshots/01.png)

![WinBlocks](screenshots/02.png)

![WinBlocks](screenshots/03.png)

![WinBlocks](screenshots/04.png)

![WinBlocks](screenshots/05.png)

![WinBlocks](screenshots/06.png)

![WinBlocks](screenshots/07.png)

![WinBlocks](screenshots/08.png)

![WinBlocks](screenshots/09.png)

![WinBlocks](screenshots/10.png)

![WinBlocks](screenshots/11.png)

![WinBlocks](screenshots/12.png)

![WinBlocks](screenshots/13.png)

![WinBlocks](screenshots/14.png)

![WinBlocks](screenshots/15.png)

![WinBlocks](screenshots/16.png)

![WinBlocks](screenshots/17.png)

![WinBlocks](screenshots/18.png)

![WinBlocks](screenshots/19.png)

![WinBlocks](screenshots/20RB.png)
**RAINBOW**

- In future any new color palettes will be added down here :D