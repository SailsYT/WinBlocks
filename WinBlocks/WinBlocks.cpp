#define SDL_MAIN_HANDLED
#define DISCORDPP_IMPLEMENTATION
#include <SDL.h>
#include <iostream>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>
#include <iterator>
#include <cstring>
#include <string>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <memory>
#include <ctime>
#include <chrono>
#include "discordpp.h"

// ============================================================================
// PROJECT DEFINITIONS
// Screen states, score entries, piece data, palette and cartridge constants.
// ============================================================================

// i wonder what would happen if you used any other rom with this?
// probs wont load but it would be intresting to see
const size_t ROM_SIZE = 32768;

enum class Screen
{
    COPYRIGHT,
    TITLE,
    GAME_MUSIC,
    SETTINGS,
    A_LEVEL,
    B_LEVEL,
    B_HIGH,
    IN_GAME,
    SCOREBOARD
};

struct HighScore
{
    std::string name;
    int score = 0;
};

struct Piece
{
    int type;
    int rot;
    int x, y;
};

SDL_Color PALETTE[4] = {
    {155, 188,  15, 255},
    {139, 172,  15, 255},
    { 48,  98,  48, 255},
    { 15,  56,  15, 255},
};

// FOR THE NEW COLOR PALETTES, as if that wasnt already obvious 
const SDL_Color PALETTE_PRESETS[20][4] =
{
    // DMG Green
    {
        {155,188,15,255}, {139,172,15,255},
        {48,98,48,255}, {15,56,15,255}
    },

    // DMG Gray
    {
        {224,224,224,255}, {162,162,162,255},
        {86,86,86,255}, {24,24,24,255}
    },

    // Miku Blue
    {
        {221,247,255,255}, {143,207,229,255},
        {58,119,167,255}, {17,44,82,255}
    },

    // Teto Red
    {
        { 255, 232, 210, 255 }, { 255, 140, 90, 255 },
        { 210, 45, 35, 255 }, { 90, 10, 10, 255 }     
    },

    // Coffee 
    {
        {255,230,176,255}, {218,173,106,255},
        {135,79,43,255}, {63,30,22,255}
    },

    // Lavender
    {
        {241,226,255,255}, {184,153,214,255},
        {104,73,147,255}, {40,27,69,255}
    },

    // FOREST
   {
       {225,239,199,255}, {150,192,114,255},
       {65,116,59,255},   {17,59,31,255}
   },

    // SUNSET
    {
        {255,222,186,255}, {253,144,117,255},
        {160,76,139,255},  {50,31,91,255}
    },


    // OCEAN 
    {
        {175,224,207,255}, {43,151,163,255},
        {9,75,108,255},    {2,22,49,255}
    },

    //  SAKURA
    {
        {255,242,246,255}, {245,182,206,255},
        {192,91,149,255},  {92,35,87,255}
    },

    // 11 - AMBER
    {
        {255,244,190,255}, {251,194,73,255},
        {176,106,22,255},  {73,45,16,255}
    },

    // 12 - MINT
    {
        {228,255,247,255}, {140,231,203,255},
        {44,153,146,255},  {12,58,78,255}
    },

    // 13 - MIDNIGHT
    {
        {201,210,249,255}, {113,126,209,255},
        {49,59,129,255},   {15,20,54,255}
    },

    // LIME
    {
        {247,255,190,255}, {195,230,75,255},
        {96,151,32,255},   {32,64,25,255}
    },

    // LIGHT PURPLE
    {
        {253,218,250,255}, {243,119,215,255},
        {114,81,198,255},  {41,31,98,255}
    },

    // PURPLE
    {
        {241,223,255,255}, {177,125,221,255},
        {95,52,152,255},   {36,17,77,255}
    },

    // PEACH
    {
        {255,244,224,255}, {250,191,154,255},
        {220,115,105,255}, {105,57,70,255}
    },

    // AUTUMN
    {
        {253,238,203,255}, {220,151,79,255},
        {138,78,43,255},   {60,42,36,255}
    },

    // CYAN
    {
        {223,255,245,255}, {99,234,186,255},
        {72,100,185,255},  {22,18,57,255}
    },

    // RAINBOW (the starting colours get updated during gameplay
    {
        {255,191,191,255}, {199,100,100,255},
        {117,23,23,255},   {48,4,4,255}
    }
};

const char* const PALETTE_NAMES[20] =
{
    "DMG GREEN",
    "DMG GRAY",
    "MIKU BLUE",
    "TETO RED",
    "COFFEE",
    "LAVENDER",
    "FOREST",
    "SUNSET",
    "OCEAN",
    "SAKURA",
    "AMBER",
    "MINT",
    "MIDNIGHT",
    "LIME",
    "LIGHT PURPLE",
    "PURPLE",
    "PEACH",
    "AUTUMN",
    "CYAN",
    "RAINBOW"
};

constexpr int PALETTE_COUNT = static_cast<int>(sizeof(PALETTE_PRESETS) / sizeof(PALETTE_PRESETS[0]));

// RAINBOW PALETTE - convert a rainbow hue into an SDL colour
SDL_Color hueToColor(float hue, float saturation, float brightness)
{
    float chroma = brightness * saturation;

    float middle = chroma * (1.0f - std::fabs(std::fmod(hue / 60.0f, 2.0f) - 1.0f));

    float offset = brightness - chroma;

    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;

    if (hue < 60.0f)
    {
        red = chroma;
        green = middle;
    }
    else if (hue < 120.0f)
    {
        red = middle;
        green = chroma;
    }
    else if (hue < 180.0f)
    {
        green = chroma;
        blue = middle;
    }
    else if (hue < 240.0f)
    {
        green = middle;
        blue = chroma;
    }
    else if (hue < 300.0f)
    {
        red = middle;
        blue = chroma;
    }
    else
    {
        red = chroma;
        blue = middle;
    }

    return {
        static_cast<Uint8>(std::round((red + offset) * 255.0f)),
        static_cast<Uint8>(std::round((green + offset) * 255.0f)),
        static_cast<Uint8>(std::round((blue + offset) * 255.0f)), 255};
}

const size_t ASCII_OFFSET = 0x415F;

const size_t ASCII_BYTES = 312;

const size_t TITLE_TILES_OFFSET = 0x4297;

const size_t MENU_TILES_OFFSET = 0x323F;

const size_t LAYOUT_ATYPE_OFFSET = 0x3E8F;

const size_t LAYOUT_COPYRIGHT_OFFSET = 0x4A07;

const size_t LAYOUT_TITLE_OFFSET = 0x4B6F;

const size_t LAYOUT_GAME_MUSIC_OFFSET = 0x4CD7;

const size_t LAYOUT_A_SELECT_OFFSET = 0x4E3F;

const size_t LAYOUT_B_SELECT_OFFSET = 0x4FA7;

const size_t PAUSE_TEXT_OFFSET = 0x2839;

const size_t GAMEOVER_TEXT_OFFSET = 0x293E;

const size_t TRYAGAIN_TEXT_OFFSET = 0x2976;

const int FIELD_W = 10;

const int FIELD_H = 18;

const int FIELD_X = 2 * 8;

const int FIELD_Y = 0;

const int PIECE_TILE[7] = { 0x84, 0x81, 0x8B, 0x83, 0x82, 0x86, 0x85 };

const char* SHAPES[7][4] = {

    { "...." "...." "###." "#...",
        "...." ".#.." ".#.." ".##.",
        "...." "..#." "###." "....",
    "...." "##.." ".#.." ".#.." },

    { "...." "...." "###." "..#.",
        "...." ".##." ".#.." ".#..",
        "...." "#..." "###." "....",
    "...." ".#.." ".#.." "##.." },

    { "...." "...." "####" "....",
        ".#.." ".#.." ".#.." ".#..",
        "...." "...." "####" "....",
    ".#.." ".#.." ".#.." ".#.." },

    { "...." "...." ".##." ".##.",
        "...." "...." ".##." ".##.",
        "...." "...." ".##." ".##.",
    "...." "...." ".##." ".##." },

    { "...." "...." "##.." ".##.",
        "...." ".#.." "##.." "#...",
        "...." "...." "##.." ".##.",
    "...." ".#.." "##.." "#..." },

    { "...." "...." ".##." "##..",
        "...." "#..." "##.." ".#..",
        "...." "...." ".##." "##..",
    "...." "#..." "##.." ".#.." },

    { "...." "...." "###." ".#..",
        "...." ".#.." ".##." ".#..",
        "...." ".#.." "###." "....",
    "...." ".#.." "##.." ".#.." }
};

const int GRAVITY_FRAMES[21] = { 53, 49, 45, 41, 37, 33, 28, 22, 17, 11,
    10,  9,  8,  7,  6,  6,  5,  5,  4,  4, 3 };

const int DIGIT_0 = 0x00;

// ============================================================================
// GAMEPLAY VALUES
// The board and the currently accumulated score, lines and level.
// ============================================================================

int field[FIELD_H][FIELD_W];

int hiddenType = 0;

int score = 0;

int lines = 0;

int level = 0;

// ============================================================================
// ROM LOADING AND GRAPHICS DATA
// Read and validate the cartridge; construct the display tile arrays.
// ============================================================================

std::vector<uint8_t> loadFile(const char* path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
    {
        std::cerr << "Could not open " << path << "\n";
        return {};
    }
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>());
}

bool romIsValid(const std::vector<uint8_t>& rom)
{
    if (rom.size() != ROM_SIZE) return false;
    if (std::memcmp(&rom[0x134], "TETRIS", 6) != 0) return false;

    uint16_t sum = 0;
    for (size_t i = 0; i < rom.size(); i++)
        if (i != 0x14E && i != 0x14F)
            sum = (uint16_t)(sum + rom[i]);
    uint16_t stored = (uint16_t)((rom[0x14E] << 8) | rom[0x14F]);
    return sum == stored;
}

std::vector<uint8_t> extractLayoutAt(const std::vector<uint8_t>& rom, size_t offset)
{
    return std::vector<uint8_t>(
        rom.begin() + offset,
        rom.begin() + offset + 360
        );
}

std::vector<uint8_t> extractLayout(const std::vector<uint8_t>& rom)
{
    return std::vector<uint8_t>(rom.begin() + LAYOUT_ATYPE_OFFSET,
        rom.begin() + LAYOUT_ATYPE_OFFSET + 360);
}

std::vector<uint8_t> extractBytes(const std::vector<uint8_t>& rom, size_t off, size_t len)
{
    return std::vector<uint8_t>(rom.begin() + off, rom.begin() + off + len);
}

std::vector<uint8_t> BuildAsciiTiles(const std::vector<uint8_t>& rom)
{
    std::vector<uint8_t> t(256 * 16, 0);

    for (size_t i = 0; i < ASCII_BYTES; i++)
    {
        t[i * 2] = rom[ASCII_OFFSET + i];
        t[i * 2 + 1] = rom[ASCII_OFFSET + i];
    }
    return t;
}

std::vector<uint8_t> BuildtitleTiles(const std::vector<uint8_t>& rom)
{
    auto t = BuildAsciiTiles(rom);

    std::memcpy(t.data() + 0x270, rom.data() + TITLE_TILES_OFFSET, 0x770);

    return t;
}

std::vector<uint8_t> buildTiles(const std::vector<uint8_t>& rom)
{
    auto t = BuildAsciiTiles(rom);

    std::memcpy(t.data() + 0x270, rom.data() + TITLE_TILES_OFFSET, 0xA0);
    std::memcpy(t.data() + 0x300, rom.data() + MENU_TILES_OFFSET, 0xD0 * 16);

    return t;
}

// ============================================================================
// HIGH-SCORE FILE STORAGE
// Save and load the existing per-level three-place high-score tables.
// ============================================================================

constexpr int HIGH_SCORE_NAME_LENGTH = 6;
void saveHighScores(const HighScore scores[10][3])
{
    std::ofstream file("winblocks_scores.txt");

    if (!file)
        return;

    for (int level = 0; level < 10; level++)
    {
        for (int rank = 0; rank < 3; rank++)
        {
            const HighScore& entry = scores[level][rank];

            file << entry.score << " " << (entry.name.empty()
                ? std::string(HIGH_SCORE_NAME_LENGTH, '-') : entry.name) << "\n";
        }
    }
}

void loadHighScores(HighScore scores[10][3])
{
    std::ifstream file("winblocks_scores.txt");

    if (!file)
        return;

    for (int level = 0; level < 10; level++)
    {
        for (int rank = 0; rank < 3; rank++)
        {
            int value;
            std::string name;

            if (!(file >> value >> name))
                return;

            if (value < 0 || value > 999999)
                return;

            if (name.size() > HIGH_SCORE_NAME_LENGTH) name.resize(HIGH_SCORE_NAME_LENGTH);

            scores[level][rank] = { name, value };
        }
    }
}

// ============================================================================
// TETROMINO AND BOARD RULES
// Collision, rotation, locking, row removal and the cartridge-inspired randomiser.
// ============================================================================

int pieceTile(int type, int rot, int n)
{
    if (type == 2)
    {
        static const int H[4] = { 0x8A, 0x8B, 0x8B, 0x8F };
        static const int V[4] = { 0x80, 0x88, 0x88, 0x89 };
        return (rot % 2 == 0) ? H[n] : V[n];
    }
    return PIECE_TILE[type];
}

bool collides(const Piece& p)
{
    const char* shape = SHAPES[p.type][p.rot];
    for (int row = 0; row < 4; row++)
        for (int col = 0; col < 4; col++)
            if (shape[row * 4 + col] == '#')
    {
        int fx = p.x + col;
        int fy = p.y + row;
        if (fx < 0 || fx >= FIELD_W || fy >= FIELD_H) return true;
        if (fy >= 0 && field[fy][fx] >= 0) return true;
    }
    return false;
}

bool tryMove(Piece& p, int dx, int dy)
{
    Piece moved = p;
    moved.x += dx;
    moved.y += dy;
    if (collides(moved)) return false;
    p = moved;
    return true;
}

bool tryRotate(Piece& p, int dir)
{
    Piece rotated = p;
    rotated.rot = (p.rot + dir + 4) % 4;
    if (collides(rotated)) return false;
    p = rotated;
    return true;
}

void lockPiece(const Piece& p)
{
    const char* shape = SHAPES[p.type][p.rot];
    int n = 0;
    for (int row = 0; row < 4; row++)
        for (int col = 0; col < 4; col++)
            if (shape[row * 4 + col] == '#')
    {
        int tile = pieceTile(p.type, p.rot, n++);
        int fx = p.x + col;
        int fy = p.y + row;
        if (fx >= 0 && fx < FIELD_W && fy >= 0 && fy < FIELD_H)
            field[fy][fx] = tile;
    }
}

bool rowIsFull(int row)
{
    for (int col = 0; col < FIELD_W; col++)
        if (field[row][col] < 0) return false;
    return true;
}

int removeFullRows()
{
    int cleared = 0;
    for (int row = FIELD_H - 1; row >= 0; )
    {
        if (!rowIsFull(row)) { row--; continue; }
        for (int r = row; r > 0; r--)
            for (int c = 0; c < FIELD_W; c++)
                field[r][c] = field[r - 1][c];
        for (int c = 0; c < FIELD_W; c++)
            field[0][c] = -1;
        cleared++;
    }
    return cleared;
}

int advancePieces(int current)
{
    int oldHidden = hiddenType;
    int rolled = 0;

    uint64_t gbCycles = static_cast<uint64_t>(
        (double)SDL_GetPerformanceCounter() * 4194304.0 /
        SDL_GetPerformanceFrequency()
        );

    for (int tries = 1; tries <= 3; tries++)
    {
        uint8_t div = (uint8_t)((gbCycles >> 8) & 0xFF);

        int steps = (div == 0) ? 256 : div;
        rolled = (steps - 1) % 7;

        if (tries == 3)
            break;

        int cur = current * 4;

        if (((oldHidden * 4 | rolled * 4 | cur) & 0xFC) != cur)
            break;

        gbCycles += 100
        + 56 * (steps - 1)
        - 4 * ((steps - 1) / 7);
    }
    hiddenType = rolled;
    return oldHidden;
}

// ============================================================================
// SCORING AND GRAVITY
// Award points, update levels and convert falling speeds into SDL time.
// ============================================================================

Uint32 dropMsForLevel(int lvl)
{
    if (lvl > 20) lvl = 20;
    return (Uint32)(GRAVITY_FRAMES[lvl] * 1000.0 / 59.73);
}

void addLines(int cleared)
{
    if (cleared <= 0) return;
    const int POINTS[5] = { 0, 40, 100, 300, 1200 };
    score += POINTS[cleared] * (level + 1);

    if (score > 999999)
    {
        score = 999999;
    }
    lines += cleared;
    if (level < 20 && lines >= (level + 1) * 10)
    {
        level++;
    }
    std::cout << "Score " << score << "  Lines " << lines << "  Level " << level << "\n";
}

void resetStats()
{
    score = 0;
    lines = 0;
    level = 0;
}

// ============================================================================
// GAME BOY GRAPHICS RENDERING
// Draw ROM tiles, menus, pieces, numbers and the locked playfield.
// ============================================================================

void drawTile(SDL_Renderer* r, const uint8_t* tile, int x, int y, const int* remap = nullptr)
{
    for (int row = 0; row < 8; row++)
    {
        uint8_t lo = tile[row * 2];
        uint8_t hi = tile[row * 2 + 1];
        for (int col = 0; col < 8; col++)
        {
            int bit = 7 - col;
            int color = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);
            if (remap) color = remap[color];
            SDL_SetRenderDrawColor(r, PALETTE[color].r, PALETTE[color].g,
                PALETTE[color].b, 255);
            SDL_RenderDrawPoint(r, x + col, y + row);
        }
    }
}

void drawSpriteTile(SDL_Renderer* r, const std::vector<uint8_t>& tiles, int idx, int x, int y)
{
    if (idx < 0 || idx >= 256)
        return;

    const uint8_t* tile = tiles.data() + idx * 16;

    for (int row = 0; row < 8; row++)
    {
        uint8_t lo = tile[row * 2];
        uint8_t hi = tile[row * 2 + 1];

        for (int col = 0; col < 8; col++)
        {
            int bit = 7 - col;

            int color =
            (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);

            if (color == 0)
                continue;

            SDL_SetRenderDrawColor(
                r,
                PALETTE[color].r, PALETTE[color].g,
                PALETTE[color].b,
                255
                );

            SDL_RenderDrawPoint(
                r,
                x + col, y + row
                );
        }
    }
}

void drawOriginalMenuSelection(SDL_Renderer* renderer, const std::vector<uint8_t>& rom, const std::vector<uint8_t>& tiles, int selection, int x, int y)
{
    const size_t labelAddresses[4] =
    {
        0x2F2F,
        0x2F38,
        0x2F41,
        0x2F4A
    };

    if (selection < 0 || selection > 3 || rom.size() != ROM_SIZE)
    {
        return;
    }

    size_t address = labelAddresses[selection];

    for (int i = 0; i < 6; i++)
    {
        uint8_t tileIndex = rom[address + i];

        drawSpriteTile(renderer, tiles, tileIndex, x + i * 8, y);
    }
}

void drawLayout(SDL_Renderer* r, const std::vector<uint8_t>& tiles,
    const std::vector<uint8_t>& layout)
{
    for (int i = 0; i < 20 * 18 && i < (int)layout.size(); i++)
        drawTile(r, tiles.data() + layout[i] * 16, (i % 20) * 8, (i / 20) * 8);
}

void drawTextBox(SDL_Renderer* r, const std::vector<uint8_t>& tiles,
    const std::vector<uint8_t>& data, int w, int h, int col, int row)
{
    if ((int)data.size() < w * h) return;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            drawTile(r, tiles.data() + data[y * w + x] * 16, (col + x) * 8, (row + y) * 8);
}

void drawField(SDL_Renderer* r, const std::vector<uint8_t>& tiles)
{
    for (int row = 0; row < FIELD_H; row++)
    {
        for (int col = 0; col < FIELD_W; col++)
        {
            int t = field[row][col];
            if (t < 0) continue;
            if ((size_t)(t + 1) * 16 > tiles.size()) continue;
            drawTile(r, tiles.data() + t * 16, FIELD_X + col * 8, FIELD_Y + row * 8);
        }
    }
}

void drawPiece(SDL_Renderer* r, const std::vector<uint8_t>& tiles, const Piece& p)
{
    const char* shape = SHAPES[p.type][p.rot];
    int n = 0;
    for (int row = 0; row < 4; row++)
        for (int col = 0; col < 4; col++)
            if (shape[row * 4 + col] == '#')
    {
        int tile = pieceTile(p.type, p.rot, n++);
        if (p.y + row < 0) continue;
        drawTile(r, tiles.data() + tile * 16,
            FIELD_X + (p.x + col) * 8, FIELD_Y + (p.y + row) * 8);
    }
}

void drawNumber(SDL_Renderer* r, const std::vector<uint8_t>& tiles,
    int value, int width, int rightX, int y)
{
    int x = rightX - 8;
    int count = 0;
    do
    {
        int digit = value % 10;
        drawTile(r, tiles.data() + (DIGIT_0 + digit) * 16, x, y);
        value /= 10;
        x -= 8;
        count++;
    } while (value > 0 || count < width);
}

void drawGBText(SDL_Renderer* renderer, const std::vector<uint8_t>& tiles, const std::string& text, int col, int row)
{
    for (int i = 0; i < (int)text.size(); i++)
    {
        char ch = text[i];
        int tile = 0x2F;

        if (ch >= '0' && ch <= '9') tile = ch - '0';

        else if (ch >= 'A' && ch <= 'Z') tile = 0x0A + (ch - 'A');

        else if (ch == '.') tile = 0x29;

        drawTile(renderer, tiles.data() + tile * 16, (col + i) * 8, row * 8);
    }
}

void drawScoreName(SDL_Renderer* renderer, const std::vector<uint8_t>& tiles, const std::string& name, int col, int row)
{
    static const char* const DASH[7] =
    {
        ".....", ".....", ".....", ".###.",
        ".....", ".....", "....."
    };

    static const char* const EXCLAMATION[7] =
    {
        "..#..", "..#..", "..#..", "..#..",
        ".....", "..#..", "....."
    };

    static const char* const QUESTION[7] =
    {
        ".###.", "#...#", "....#", "...#.",
        "..#..", ".....", "..#.."
    };

    const int originX = col * 8;
    const int originY = row * 8;

    for (int i = 0; i < HIGH_SCORE_NAME_LENGTH; ++i)
    {
        char ch = (i < static_cast<int>(name.size())) ? name[i] : '-';

        int x = originX + i * 8;

        if (ch == '-' || ch == '!' || ch == '?')
        {
            const char* const* glyph = (ch == '-') ? DASH : ((ch == '!') ? EXCLAMATION : QUESTION);

            SDL_SetRenderDrawColor(renderer, PALETTE[0].r, PALETTE[0].g, PALETTE[0].b, 255);

            SDL_Rect background = { x, originY, 8, 8 };
            SDL_RenderFillRect(renderer, &background);

            SDL_SetRenderDrawColor(renderer, PALETTE[3].r, PALETTE[3].g, PALETTE[3].b, 255);

            for (int py = 0; py < 7; ++py)
            {
                for (int px = 0; px < 5; ++px)
                {
                    if (glyph[py][px] == '#')
                    {
                        SDL_RenderDrawPoint(renderer, x + 1 + px, originY + py);
                    }
                }
            }
            continue;
        }

        // Reuse the OG Rom characters
        int tileIndex = 0x2F;

        if (ch >= 'A' && ch <= 'Z')
            tileIndex = 0x0A + (ch - 'A');
        else if (ch >= '0' && ch <= '9')
            tileIndex = ch - '0';
        else if (ch == '.')
            tileIndex = 0x29;

        if ((tileIndex + 1) * 16 > static_cast<int>(tiles.size()))
        {
            continue;
        }

        const uint8_t* tile = tiles.data() + tileIndex * 16;

        for (int py = 0; py < 8; ++py)
        {
            uint8_t lo = tile[py * 2];
            uint8_t hi = tile[py * 2 + 1];

            for (int px = 0; px < 8; ++px)
            {
                int bit = 7 - px;
                int shade = (((hi >> bit) & 1) << 1) | ((lo >> bit) & 1);
                SDL_SetRenderDrawColor(renderer, PALETTE[shade].r, PALETTE[shade].g, PALETTE[shade].b, 255);

                SDL_RenderDrawPoint(renderer, x + px, originY + py
                );
            }
        }
    }
}

void drawGhostPiece(SDL_Renderer* renderer, const Piece& ghost)
{
    const char* shape = SHAPES[ghost.type][ghost.rot];

    SDL_SetRenderDrawColor(renderer, PALETTE[2].r, PALETTE[2].g, PALETTE[2].b, 255);

    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            if (shape[row * 4 + col] != '#')
                continue;

            int boardY = ghost.y + row;

            if (boardY < 0 || boardY >= FIELD_H)
                continue;

            SDL_Rect outline =
            {
                FIELD_X + (ghost.x + col) * 8 + 1,
                FIELD_Y + boardY * 8 + 1,
                6,
                6
            };

            SDL_RenderDrawRect(renderer, &outline);
        }
    }
}

void drawPreviewLabel(SDL_Renderer* renderer, const std::vector<uint8_t>& tiles, const char* label, int x, int y)
{
    // draw four letters using le OG ROM tiles
    for (int i = 0; i < 4; ++i)
    {
        int tileIndex = 0x0A + (label[i] - 'A');

        drawTile(renderer, tiles.data() + tileIndex * 16, x + i * 8, y);
    }
}

void drawHeldPreview(SDL_Renderer* renderer, const std::vector<uint8_t>& tiles, const std::vector<uint8_t>& layout, int heldType, bool showLabel)
{
    if (layout.size() < 20 * 18 ||
        tiles.size() < 256 * 16)
    {
        return;
    }

    // copy the next piece preview box
    for (int row = 12; row < 18; ++row)
    {
        for (int col = 14; col < 20; ++col)
        {
            int tileIndex = layout[row * 20 + col];

            drawTile(renderer, tiles.data() + tileIndex * 16, (col - 13) * 8, (row - 12) * 8);
        }
    }

    // Display held during gameplay, hide it during game over 
    if (showLabel)
    {
        drawPreviewLabel(renderer, tiles, "HELD", 16, 8);
    }

    // if no piece is held, leave the box empty
    if (heldType < 0 || heldType >= 7)
        return;

    // draw the stored tetromino
    const char* shape = SHAPES[heldType][0];
    int blockNumber = 0;

    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            if (shape[row * 4 + col] != '#')
                continue;

            int tileIndex = pieceTile(heldType, 0, blockNumber++);

            int x = FIELD_X + col * 8;
            int y = 8 + row * 8;

            drawTile(renderer, tiles.data() + tileIndex * 16, x, y);
        }
    }
}

void drawMirroredRomTile(
    SDL_Renderer* renderer,
    const uint8_t* tile,
    int x,
    int y)
{
    for (int row = 0; row < 8; ++row)
    {
        uint8_t lo = tile[row * 2];
        uint8_t hi = tile[row * 2 + 1];

        for (int col = 0; col < 8; ++col)
        {
            int bit = col;

            int color =
                (((hi >> bit) & 1) << 1) |
                ((lo >> bit) & 1);

            SDL_SetRenderDrawColor(
                renderer,
                PALETTE[color].r,
                PALETTE[color].g,
                PALETTE[color].b,
                255
            );

            SDL_RenderDrawPoint(
                renderer,
                x + col,
                y + row
            );
        }
    }
}

void drawGameTimer(SDL_Renderer* renderer, const std::vector<uint8_t>& tiles, 
    const std::vector<uint8_t>& layout, uint64_t elapsedMs)
{
    if (layout.size() < 20 * 18 || tiles.size() < 256 * 16)
        return;

    SDL_SetRenderDrawColor(renderer, PALETTE[3].r, PALETTE[3].g, PALETTE[3].b, 255);

    SDL_RenderClear(renderer);


    // copy the OG level box from the rom
    for (int row = 5; row <= 8; ++row)
    {
        int rightBorderTile = layout[row * 20 + 19];

        drawMirroredRomTile(renderer, tiles.data() + rightBorderTile * 16, 0, (row - 5) * 8);

        for (int col = 14; col <= 19; ++col)
        {
            int tileIndex = layout[row * 20 + col];

            if (row == 6 && col >= 14 && col <= 18)
            {
                tileIndex = 0x2F;
            }

            drawTile(renderer, tiles.data() + tileIndex * 16, (col - 13) * 8, (row - 5) * 8);
        }
    }

    uint64_t totalSeconds = elapsedMs / 1000;

    // after the player passes 59:59, display hours and minutes
    bool showHours = totalSeconds >= 3599;

    // draw the tile using the OG font
    drawPreviewLabel(renderer, tiles, showHours ? "HOUR" : "TIME", 12, 8);

    int firstNumber;
    int secondNumber;

    if (showHours)
    {
        // hours and minutes 
        firstNumber = static_cast<int>(std::min<uint64_t>(totalSeconds / 3600, 99));

        secondNumber = static_cast<int>((totalSeconds / 60) % 60);
    }
    else
    {
        // minutes and seconds
        firstNumber = static_cast<int>(totalSeconds / 60);
        secondNumber = static_cast<int>(totalSeconds % 60);
    }

    const int digits[4] =
    {
        firstNumber / 10,
        firstNumber % 10,
        secondNumber / 10,
        secondNumber % 10
    };

    const int digitX[4] = { 8, 16, 32, 40 };

    // draw the four digits with OG rom tiles
    for (int i = 0; i < 4; ++i)
    {
        drawTile(renderer, tiles.data() + digits[i] * 16, digitX[i], 16);
    }

    // draw the : between minutes and seconds
    SDL_SetRenderDrawColor(renderer, PALETTE[3].r, PALETTE[3].g, PALETTE[3].b, 255);

    SDL_RenderDrawPoint(renderer, 27, 18);
    SDL_RenderDrawPoint(renderer, 27, 22);
}


// ============================================================================
// WINDOW SCALING
// Maintain the original 160 x 144 display aspect ratio.
// ============================================================================

SDL_Rect getGameDstRect(SDL_Renderer* renderer)
{
    int windowW, windowH;
    SDL_GetRendererOutputSize(renderer, &windowW, &windowH);

    float scaleX = windowW / 160.0f;
    float scaleY = windowH / 144.0f;
    float scale = (scaleX < scaleY) ? scaleX : scaleY;

    int w = (int)std::floor(160.0f * scale);
    int h = (int)std::floor(144.0f * scale);

    int x = (windowW - w) / 2;
    int y = (windowH - h) / 2;

    return { x, y, w, h };
}

// ============================================================================
// SOUND CHANNEL 1 AND 2: PULSE
// Pulse duty, frequency sweep, envelope and length-counter emulation.
// ============================================================================

struct PulseChannel
{
    uint16_t period = 0x06D6;
    int duty = 2;
    float volume = 0.08f;
    double phase = 0.0;

    uint8_t nr10 = 0;
    uint16_t shadowPeriod = 0;
    int sweepTimer = 8;
    bool sweepEnabled = false;
    bool enabled = true;
    double sweepClock = 0.0;

    uint8_t nr12 = 0xF0;
    int envelopeVolume = 15;
    int envelopeTimer = 8;
    bool envelopeRunning = false;
    double envelopeClock = 0.0;

    uint8_t nr11 = 0x80;
    uint8_t nr14 = 0x00;
    int lengthCounter = 0;
    double lengthClock = 0.0;

    int sweepPace() const
    {
        return (nr10 >> 4) & 7;
    }
    int sweepShift() const
    {
        return nr10 & 7;
    }
    int calculateSweep() const
    {
        int change = shadowPeriod >> sweepShift();

        if (nr10 & 0x08) return shadowPeriod - change;

        return shadowPeriod + change;
    }
    void writeNR11(uint8_t value)
    {
        nr11 = value;

        duty = (nr11 >> 6) & 0x03;

        lengthCounter = 64 - (nr11 & 0x3F);
    }
    void writeNR13(uint8_t value)
    {
        period = (period & 0x0700) | value;
    }
    void trigger()
    {
        enabled = true;
        if (lengthCounter == 0)
        {
            lengthCounter = 64;
        }
        shadowPeriod = period;

        sweepTimer = sweepPace() == 0 ? 8 : sweepPace();
        sweepEnabled = sweepPace() != 0 || sweepShift() != 0;
        sweepClock = 0.0;

        if (sweepShift() != 0 && calculateSweep() > 2047) enabled = false;

        envelopeVolume = (nr12 >> 4) & 0x0F;

        int pace = nr12 & 0x07;

        envelopeTimer = pace == 0 ? 8 : pace;
        envelopeRunning = pace != 0;

        envelopeClock = 0.0;

        if ((nr12 & 0xF8) == 0)
        {
            enabled = false;
        }
    }
    void writeNR14(uint8_t value)
    {
        nr14 = value;

        period = (period & 0x00FF) | ((nr14 & 0x07) << 8);

        if (nr14 & 0x80)
        {
            trigger();
        }
    }
    void clockSweep()
    {
        if (--sweepTimer > 0) return;

        sweepTimer = sweepPace() == 0 ? 8 : sweepPace();

        if (!enabled || !sweepEnabled || sweepPace() == 0) return;

        int newPeriod = calculateSweep();

        if (newPeriod > 2047)
        {
            enabled = false;
            return;
        }

        if (sweepShift() != 0)
        {
            shadowPeriod = period = static_cast<uint16_t>(newPeriod);

            if (calculateSweep() > 2047) enabled = false;
        }
    }

    void clockEnvelope()
    {
        if (!envelopeRunning) return;

        envelopeTimer--;

        if (envelopeTimer > 0) return;

        int pace = nr12 & 0x07;
        envelopeTimer = pace == 0 ? 8 : pace;

        int direction = (nr12 & 0x08) ? 1 : -1;

        int newVolume = envelopeVolume + direction;

        if (newVolume >= 0 && newVolume <= 15)
        {
            envelopeVolume = newVolume;
        }
        else
        {
            envelopeRunning = false;
        }
    }

    void clockLength()
    {
        if ((nr14 & 0x40) == 0) return;

        if (lengthCounter == 0) return;

        lengthCounter--;

        if (lengthCounter == 0)
        {
            enabled = false;
        }
    }

    float nextSample(int sampleRate)
    {
        sweepClock += 128.0 / sampleRate;

        if (sweepClock >= 1.0)
        {
            sweepClock -= 1.0;
            clockSweep();
        }

        envelopeClock += 64.0 / sampleRate;

        if (envelopeClock >= 1.0)
        {
            envelopeClock -= 1.0;
            clockEnvelope();
        }

        lengthClock += 256.0 / sampleRate;

        while (lengthClock >= 1.0)
        {
            lengthClock -= 1.0;
            clockLength();
        }

        if (!enabled) return 0.0f;

        static const int DUTY_PATTERN[4][8] =
        {
            {0,0,0,0,0,0,0,1},
            {1,0,0,0,0,0,0,1},
            {1,0,0,0,0,1,1,1},
            {0,1,1,1,1,1,1,0}
        };

        double frequency = 131072.0 / (2048.0 - period);

        int step = static_cast<int>(phase * 8.0);

        float amplitude = volume * (envelopeVolume / 15.0f);

        float sample = DUTY_PATTERN[duty][step] ? amplitude : -amplitude;

        phase += frequency / sampleRate;

        if (phase >= 1.0) phase -= std::floor(phase);

        return sample;
    }
};

// ============================================================================
// SOUND CHANNEL 3: WAVE
// ROM waveform playback, frequency and output level handling.
// ============================================================================

struct WaveChannel
{
    uint8_t nr30 = 0;
    uint8_t nr32 = 0;
    uint8_t nr34 = 0;

    uint16_t period = 0;

    int lengthCounter = 0;
    double lengthClock = 0.0;
    double wavePosition = 0.0;

    bool enabled = false;
    float volume = 0.08f;

    uint8_t waveRAM[16] =
    {
        0x11, 0x23, 0x56, 0x78,
        0x99, 0x98, 0x76, 0x67,
        0x9A, 0xDF, 0xFE, 0xC9,
        0x85, 0x42, 0x11, 0x31
    };

    void writeNR30(uint8_t value)
    {
        nr30 = value;

        if ((nr30 & 0x80) == 0)
            enabled = false;
    }

    void writeNR31(uint8_t value)
    {
        lengthCounter = 256 - value;
    }

    void writeNR32(uint8_t value)
    {
        nr32 = value;
    }

    void writeNR33(uint8_t value)
    {
        period = (period & 0x0700) | value;
    }

    void writeNR34(uint8_t value)
    {
        nr34 = value;

        period = (period & 0x00FF) | ((nr34 & 0x07) << 8);

        if (nr34 & 0x80)
        {
            enabled = (nr30 & 0x80) != 0;

            wavePosition = 0.0;
            lengthClock = 0.0;

            if (lengthCounter == 0) lengthCounter = 256;
        }
    }
    float nextSample(int sampleRate)
    {
        lengthClock += 256.0 / sampleRate;

        while (lengthClock >= 1.0)
        {
            lengthClock -= 1.0;

            if ((nr34 & 0x40) && lengthCounter > 0)
            {
                lengthCounter--;

                if (lengthCounter == 0)
                    enabled = false;
            }
        }

        if (!enabled)
            return 0.0f;

        int level = (nr32 >> 5) & 3;
        float sample = 0.0f;

        if (level != 0)
        {
            int index = static_cast<int>(wavePosition);

            uint8_t packed = waveRAM[index / 2];

            int nibble = (index % 2 == 0) ? (packed >> 4) : (packed & 0x0F);

            int shift = level - 1;
            int reducedSample = nibble >> shift;

            float center = (15 >> shift) / 2.0f;

            sample = ((reducedSample - center) / 7.5f) * volume;
        }
        double frequency = 65536.0 / (2048.0 - period);

        wavePosition += (frequency * 32.0) / sampleRate;

        if (wavePosition >= 32.0) wavePosition = std::fmod(wavePosition, 32.0);

        return sample;
    }
};

// ============================================================================
// SOUND CHANNEL 4: NOISE
// Noise LFSR and envelope/length emulation.
// ============================================================================

struct NoiseChannel
{
    uint8_t nr42 = 0;
    uint8_t nr43 = 0;
    uint8_t nr44 = 0;

    uint16_t lfsr = 0x7FFF;

    int lengthCounter = 0;
    double lengthClock = 0.0;

    int envelopeVolume = 0;
    int envelopeTimer = 8;
    bool envelopeRunning = false;
    double envelopeClock = 0.0;

    bool enabled = false;
    double noiseClock = 0.0;
    float volume = 0.08f;

    void writeNR41(uint8_t value)
    {
        lengthCounter = 64 - (value & 0x3F);
    }
    void writeNR42(uint8_t value)
    {
        nr42 = value;

        if ((nr42 & 0xF8) == 0) enabled = false;
    }
    void writeNR43(uint8_t value)
    {
        nr43 = value;
    }
    void trigger()
    {
        enabled = (nr42 & 0xF8) != 0;

        if (lengthCounter == 0) lengthCounter = 64;

        envelopeVolume = (nr42 >> 4) & 0x0F;

        int pace = nr42 & 0x07;
        envelopeTimer = pace == 0 ? 8 : pace;
        envelopeRunning = pace != 0;

        lfsr = 0x7FFF;

        noiseClock = 0.0;
        envelopeClock = 0.0;
        lengthClock = 0.0;
    }
    void writeNR44(uint8_t value)
    {
        nr44 = value;

        if (nr44 & 0x80) trigger();
    }
    void clockEnvelope()
    {
        if (!envelopeRunning) return;

        if (--envelopeTimer > 0) return;

        int pace = nr42 & 0x07;
        envelopeTimer = pace == 0 ? 8 : pace;

        int direction = (nr42 & 0x08) ? 1 : -1;
        int newVolume = envelopeVolume + direction;

        if (newVolume >= 0 && newVolume <= 15)
            envelopeVolume = newVolume;
        else envelopeRunning = false;
    }
    void clockLength()
    {
        if ((nr44 & 0x40) && lengthCounter > 0)
        {
            if (--lengthCounter == 0) enabled = false;
        }
    }
    float nextSample(int sampleRate)
    {
        envelopeClock += 64.0 / sampleRate;

        while (envelopeClock >= 1.0)
        {
            envelopeClock -= 1.0;
            clockEnvelope();
        }

        lengthClock += 256.0 / sampleRate;

        while (lengthClock >= 1.0)
        {
            lengthClock -= 1.0;
            clockLength();
        }

        if (!enabled) return 0.0f;

        int shift = (nr43 >> 4) & 0x0F;
        int dividerBits = nr43 & 0x07;

        double divider = dividerBits == 0 ? 0.5 : dividerBits;

        double ticksPerSecond = shift >= 14 ? 0.0 : 262144.0 / (divider * (1u << shift));

        noiseClock += ticksPerSecond / sampleRate;

        while (noiseClock >= 1.0)
        {
            noiseClock -= 1.0;

            int feedback = (lfsr & 1) ^ ((lfsr >> 1) & 1);

            lfsr = (lfsr >> 1) | (feedback << 14);

            if (nr43 & 0x08)
            {
                lfsr = (lfsr & ~(1u << 6)) | (feedback << 6);
            }
        }
        float amplitude = volume * (envelopeVolume / 15.0f);

        return (lfsr & 1) ? -amplitude : amplitude;
    }
};

// ============================================================================
// APPLICATION ENTRY POINT
// Initialise services, run menu/gameplay/audio updates and display each frame.
// ============================================================================

int main()
{

    std::vector<uint8_t> titleTiles;

    // ============================================================================
    // SDL WINDOW AND FRAMEBUFFER
    // Create the display window, renderer and 160 x 144 target texture.
    // ============================================================================

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0)
    {
        std::cout << "SDL failed to start: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "WinBlocks",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1600, 900,
        SDL_WINDOW_SHOWN);

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    SDL_Texture* gameFrame = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        160, 144
        );

    if (!gameFrame)
    {
        SDL_Log("Failed to create game frame texture: %s", SDL_GetError());
        return 1;
    }

    // create a second render target for the held piece box 
    SDL_Texture* heldFrame = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 56, 48);

    if (!heldFrame)
    {
        SDL_Log("Could not create Hold preview texture: %s", SDL_GetError());
    }

    SDL_Texture* gameTimerFrame = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 56, 28);

    if (!gameTimerFrame)
    {
        SDL_Log("Could not create gameplay timer texture: %s", SDL_GetError());
    }

    // ============================================================================
    // AUDIO DEVICE
    // Open the queued 48 kHz, mono, floating-point audio output.
    // ============================================================================

    SDL_AudioSpec wanted{};
    wanted.freq = 48000;
    wanted.format = AUDIO_F32SYS;
    wanted.channels = 1;
    wanted.samples = 1024;
    wanted.callback = nullptr;

    SDL_AudioDeviceID audioDevice = SDL_OpenAudioDevice(nullptr, 0, &wanted, nullptr, 0);

    if (audioDevice == 0)
    {
        SDL_Log("Audio failed: %s", SDL_GetError());
    }
    else
    {
        SDL_Log("Audio device opened successfully!");
    }

    PulseChannel pulse1;
    PulseChannel pulse2;
    WaveChannel wave;
    NoiseChannel noise;

    pulse1.enabled = false;
    pulse2.enabled = false;
    wave.enabled = false;
    noise.enabled = false;

    const int AUDIO_SAMPLE_RATE = 48000;
    const int AUDIO_CHUNK_SAMPLES = 512;
    const Uint32 AUDIO_TARGET_BYTES = 2048 * sizeof(float);

    std::vector<float> audioBuffer(AUDIO_CHUNK_SAMPLES);

    if (audioDevice != 0)
    {
        SDL_PauseAudioDevice(audioDevice, 0);
    }

    // ============================================================================
    // ROM MUSIC STATE
    // Independent music cursors and deadlines for the four Game Boy channels.
    // ============================================================================

    bool musicPreviewPlaying = false;

    int currentRomSong = 0;
    int selectedGameType = 0;
    int selectedMusic = 0;

    bool musicMenuFocus = false;

    size_t musicTempoTable = 0x6F0E;

    size_t musicSectionPointer = 0x7142;
    size_t musicCursor = 0x7168;

    size_t music1SectionPointer = 0x714C;
    size_t music1Cursor = 0x71CB;

    int music1TempoFrames = 12;
    double nextMusic1NoteAt = 0;

    uint8_t music1Envelope = 0x43;
    uint8_t music1DutyLength = 0x81;

    uint16_t music1CurrentPeriod = 0;
    bool music1NoteOn = false;

    Uint32 rotationSfxUntil = 0;

    int musicTempoFrames = 12;
    double nextMusicNoteAt = 0;

    size_t music3SectionPointer = 0x7156;
    size_t music3Cursor = 0x723A;

    int music3TempoFrames = 12;
    double nextMusic3NoteAt = 0;

    size_t music4SectionPointer = 0x7162;
    size_t music4Cursor = 0x72A3;

    int music4TempoFrames = 12;
    double nextMusic4NoteAt = 0;

    Uint32 noiseSfxUntil = 0;

    pulse2.nr10 = 0x00;
    pulse2.nr12 = 0xF0;
    pulse2.writeNR11(0x80);
    pulse2.enabled = false;

    int startupChimeStep = 0;
    Uint32 startupChimeAt = SDL_GetTicks() + 300;

    bool menuMoveSecondNotePending = false;
    Uint32 menuMoveSecondNoteAt = 0;

    auto playMusic1Note = [&]()
    {
        if (!musicPreviewPlaying || !music1NoteOn)
        {
            pulse1.enabled = false;
            return;
        }

        pulse1.nr10 = 0x00;
        pulse1.nr12 = music1Envelope;

        pulse1.writeNR11(music1DutyLength);

        pulse1.writeNR13(static_cast<uint8_t>(music1CurrentPeriod & 0xFF));

        pulse1.writeNR14(static_cast<uint8_t>(0x80 | ((music1CurrentPeriod >> 8) & 0x07)));
    };

    // ============================================================================
    // MENU SOUND EFFECTS
    // The existing menu pulse effects and line-clear pulse helper.
    // ============================================================================

    auto playMenuSound = [&](bool confirm)
    {
        Uint32 now = SDL_GetTicks();

        rotationSfxUntil = now + (confirm ? 55 : 168);

        menuMoveSecondNotePending = !confirm;
        menuMoveSecondNoteAt = now + 84;

        pulse1.nr10 = 0x00;

        pulse1.writeNR11(confirm ? 0xB6 : 0xB5);
        pulse1.nr12 = confirm ? 0xA1 : 0xD0;
        pulse1.writeNR13(confirm ? 0x80 : 0x40);
        pulse1.writeNR14(0xC7);
    };

    auto playLinePulse = [&](uint8_t nr10,
        uint8_t nr11,
        uint8_t nr12,
        uint8_t nr13,
        uint8_t nr14)
    {
        pulse1.nr10 = nr10;
        pulse1.writeNR11(nr11);
        pulse1.nr12 = nr12;
        pulse1.writeNR13(nr13);
        pulse1.writeNR14(nr14);
    };

    // ============================================================================
    // CONTROLLER INPUT
    // Discover compatible controllers and track their instance IDs.
    // ============================================================================

    SDL_GameController* controller = nullptr;
    SDL_JoystickID controllerId = -1;


    // Track Trigger Clicks
    bool leftTriggerHeld = false;
    bool rightTriggerHeld = false;

    for (int i = 0; i < SDL_NumJoysticks(); i++)
    {
        if (SDL_IsGameController(i))
        {
            controller = SDL_GameControllerOpen(i);

            if (controller)
            {
                controllerId = SDL_JoystickInstanceID(
                    SDL_GameControllerGetJoystick(controller)
                    );

                SDL_Log("Controller connected: %s",
                    SDL_GameControllerName(controller));

                break;
            }
        }
    }

    auto padHeld = [&](SDL_GameControllerButton button) -> bool
    {
        return controller && SDL_GameControllerGetButton(controller, button) != 0;
    };

    // ============================================================================
    // CARTRIDGE ASSETS AND SELECTION STATE
    // Screen layouts, leaderboard input and ROM pointers.
    // ============================================================================

    std::vector<uint8_t> tiles;
    std::vector<uint8_t> layout;
    std::vector<uint8_t> musicRom;
    std::vector<uint8_t> copyrightLayout;
    std::vector<uint8_t> titleLayout;
    std::vector<uint8_t> gameMusicLayout;
    std::vector<uint8_t> expandedGameMusicLayout;
    std::vector<uint8_t> extrasLayout;
    std::vector<uint8_t> extrasAudioLayout;
    std::vector<uint8_t> aSelectLayout;
    std::vector<uint8_t> bSelectLayout;
    std::vector<uint8_t> pauseText, gameOverText, tryAgainText;

    bool romLoaded = false;

    Screen screen = Screen::COPYRIGHT;
    Uint32 screenStart = SDL_GetTicks();

    Screen settingsReturnScreen = Screen::TITLE;
    int settingsSelection = 0;

    // 0 = Page one, 1 = Page two
    int settingsPage = 0;

    // =====================================================
    // WINBLOCKS EXTRAS - LOAD AND SAVE
    // =====================================================

    std::string settingsPath;

    // Find the Windows application preferences folder
    if (char* directory = SDL_GetPrefPath("SailsYT", "WinBlocks"))
    {
        settingsPath = std::string(directory) + "winblocks_settings.ini";

        SDL_free(directory);
    }
    else
    {
        SDL_Log("Settings folder error: %s", SDL_GetError());
    }

    int paletteIndex = 0;
    bool startFullscreen = false;
    bool ghostEnabled = false;
    bool hardDropEnabled = false;
    bool holdEnabled = false;

    // for the playtime tracking, idk why im adding this, but i am :)
    uint64_t savedPlaySeconds = 0;
    uint64_t sessionPlayMs = 0;
    uint64_t gamePlayMs = 0;

    // for the volume selcection
    int masterVolume = 100;
    int musicVolume = 100;
    int sfxVolume = 100;

    // Load existing preferences if available
    if (!settingsPath.empty())
    {
        std::ifstream file(settingsPath);
        std::string line;

        while (std::getline(file, line))
        {
            size_t equals = line.find('=');

            if (equals == std::string::npos)
                continue;

            std::string key = line.substr(0, equals);

            std::istringstream valueText(line.substr(equals + 1));

            // read the playtime (as 64 bit)
            if (key == "TotalPlaySeconds")
            {
                uint64_t seconds = 0;

                if (valueText >> seconds)
                    savedPlaySeconds = seconds;

                continue;
            }

            // existing settings use the normal integers 
            int value = 0;

            if (!(valueText >> value))
                continue;


            if (key == "Palette" && value >= 0 && value < PALETTE_COUNT)
            {
                paletteIndex = value;
            }
            else if (key == "Fullscreen" && (value == 0 || value == 1))
            {
                startFullscreen = (value == 1);
            }

            else if (key == "GhostPiece" && (value == 0 || value == 1))
            {
                ghostEnabled = (value == 1);
            }
            else if (key == "HardDrop" && (value == 0 || value == 1))
            {
                hardDropEnabled = (value == 1);
            }
            else if (key == "HoldPiece" && (value == 0 || value == 1))
            {
                holdEnabled = (value == 1);
            }
            else if (key == "MasterVolume" && value >= 0 && value <= 100)
            {
                masterVolume = value;
            }
            else if (key == "MusicVolume" && value >= 0 && value <= 100)
            {
                musicVolume = value;
            }
            else if (key == "SfxVolume" && value >= 0 && value <= 100)
            {
                sfxVolume = value;
            }
        }
    }

    // Apply the saved palette instantly
    for (int i = 0; i < 4; ++i)
    {
        PALETTE[i] = PALETTE_PRESETS[paletteIndex][i];
    }

    // Restore fullscreen if previously enabled
    if (startFullscreen)
    {
        if (SDL_SetWindowFullscreen(
            window,
            SDL_WINDOW_FULLSCREEN_DESKTOP) != 0)
        {
            SDL_Log("Fullscreen restore error: %s", SDL_GetError());
        }
    }

    // Save the current display settings
    auto savePreferences = [&](){
            if (settingsPath.empty())
                return;

            std::ofstream file(settingsPath);

            if (!file)
            {
                SDL_Log("Could not save WinBlocks settings.");
                return;
            }

            bool fullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
            file << "[Display]\n";
            file << "Palette=" << paletteIndex << '\n';
            file << "Fullscreen=" << (fullscreen ? 1 : 0) << '\n';
            file << "[Gameplay]\n";
            file << "GhostPiece=" << (ghostEnabled ? 1 : 0) << '\n';
            file << "HardDrop=" << (hardDropEnabled ? 1 : 0) << '\n';
            file << "HoldPiece=" << (holdEnabled ? 1 : 0) << '\n';

            file << "[Audio]\n";
            file << "MasterVolume=" << masterVolume << '\n';
            file << "MusicVolume=" << musicVolume << '\n';
            file << "SfxVolume=" << sfxVolume << '\n';

            file << "[Statistics]\n";
            file << "TotalPlaySeconds=" << (savedPlaySeconds + sessionPlayMs / 1000) << '\n';
        };

    // All palette shortcuts now save automatically
    auto cyclePalette = [&](int direction)
        {
            paletteIndex = (paletteIndex + direction + PALETTE_COUNT) % PALETTE_COUNT;

            for (int i = 0; i < 4; ++i)
            {
                PALETTE[i] = PALETTE_PRESETS[paletteIndex][i];
            }
            savePreferences();
        };

    // Shared fullscreen handler for F11 and EXTRAS
    auto toggleFullscreen = [&](){
            bool fullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;

            if (SDL_SetWindowFullscreen(window, fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP) != 0)
            {
                SDL_Log("Fullscreen error: %s", SDL_GetError());
                return;
            }
            savePreferences();
        };

    int numPlayers = 0;
    int aLevel = 0;

    HighScore highScores[10][3]{};
    loadHighScores(highScores);
    int playedLevel = 0;
    int pendingRank = -1;
    int namePosition = 0;

    auto readMusicWord = [&](size_t address) -> uint16_t
    {
        if (address + 1 >= musicRom.size()) return 0;

        return static_cast<uint16_t>(musicRom[address] | (static_cast<uint16_t>(musicRom[address + 1]) << 8));
    };

    // ============================================================================
    // SONG PLAYBACK CONTROL
    // Stop, initialise and restore music selected from the cartridge.
    // ============================================================================

    auto stopRomSong = [&]()
    {
        musicPreviewPlaying = false;
        currentRomSong = 0;

        pulse1.enabled = false;
        pulse2.enabled = false;
        wave.enabled = false;
        noise.enabled = false;

        music1NoteOn = false;
        rotationSfxUntil = 0;
        noiseSfxUntil = 0;
    };

    auto startRomSong = [&](int songNumber)
    {
        if (!romLoaded || musicRom.size() != ROM_SIZE)
        {
            SDL_Log("Cannot play music: ROM not loaded.");
            return;
        }

        if (songNumber != 1 &&
            songNumber != 3 &&
            songNumber != 4 &&
            songNumber != 5 &&
            songNumber != 6 &&
            songNumber != 7)
        {
            return;
        }

        const size_t songTable = 0x64B0;

        size_t header = readMusicWord(songTable + (songNumber - 1) * 2);

        if (header + 10 >= musicRom.size())
            return;

        size_t tempoTable = readMusicWord(header + 1);

        size_t sq1Table = readMusicWord(header + 3);
        size_t sq2Table = readMusicWord(header + 5);
        size_t waveTable = readMusicWord(header + 7);
        size_t noiseTable = readMusicWord(header + 9);

        if (tempoTable >= musicRom.size() ||
            sq1Table == 0 || sq1Table + 1 >= musicRom.size() ||
            sq2Table == 0 || sq2Table + 1 >= musicRom.size() ||
            (waveTable != 0 && waveTable + 1 >= musicRom.size()) ||
            (noiseTable != 0 && noiseTable + 1 >= musicRom.size()))
        {
            SDL_Log("Invalid ROM song header.");
            return;
        }

        stopRomSong();

        currentRomSong = songNumber;
        musicTempoTable = tempoTable;

        Uint32 startTime = SDL_GetTicks();

        music1SectionPointer = sq1Table;
        music1Cursor = readMusicWord(sq1Table);
        music1TempoFrames = 12;
        nextMusic1NoteAt = startTime;

        music1Envelope = 0x43;
        music1DutyLength = 0x81;
        music1NoteOn = false;

        musicSectionPointer = sq2Table;
        musicCursor = readMusicWord(sq2Table);
        musicTempoFrames = 12;
        nextMusicNoteAt = startTime;

        music3SectionPointer = waveTable;
        music3Cursor = waveTable != 0 ? readMusicWord(waveTable) : 0;
        music3TempoFrames = 12;
        nextMusic3NoteAt = startTime;

        wave.writeNR30(0x00);

        music4SectionPointer = noiseTable; music4Cursor = noiseTable != 0 ? readMusicWord(noiseTable) : 0;
        music4TempoFrames = 12;
        nextMusic4NoteAt = startTime;

        musicPreviewPlaying = true;

        SDL_Log("Starting Tetris ROM Song %d", songNumber);
    };

    auto resumeChosenMusic = [&]()
    {
        if (selectedMusic == 3)
        {
            stopRomSong();
        }
        else
        {
            startRomSong(5 + selectedMusic);
        }
    };

    // ============================================================================
    // ROM ASSET INITIALISATION
    // Load game layouts, tile sets and overlay graphics.
    // ============================================================================

    auto tryLoadRom = [&](const char* path) {
        std::vector<uint8_t> rom = loadFile(path);
        if (!romIsValid(rom))
        {
            SDL_SetWindowTitle(window, "WinBlocks - drag tetris.gb onto this window");
            return;
        }
        copyrightLayout = extractLayoutAt(rom, LAYOUT_COPYRIGHT_OFFSET);
        titleLayout = extractLayoutAt(rom, LAYOUT_TITLE_OFFSET);
        gameMusicLayout = extractLayoutAt(rom, LAYOUT_GAME_MUSIC_OFFSET);

        // gonna expand the Gane Type menu using the ROM tiles
        expandedGameMusicLayout = gameMusicLayout;

        // Music selection gets moved down by one tile row
        for (int row = 9; row <= 15; ++row)
        {
            for (int col = 1; col <= 18; ++col)
            {
                expandedGameMusicLayout[(row + 1) * 20 + col] = gameMusicLayout[row * 20 + col];
            }
        }

        // This will build a second Game Type Row
        // I'm Adding a divider that comes from the Music Type Panel 
        for (int col = 2; col <= 17; ++col)
        {
            expandedGameMusicLayout[6 * 20 + col] = gameMusicLayout[13 * 20 + col];
            expandedGameMusicLayout[7 * 20 + col] = gameMusicLayout[5 * 20 + col];
            expandedGameMusicLayout[8 * 20 + col] = gameMusicLayout[6 * 20 + col];
        }

        // Create a gap B4 the Music Type header
        for (int col = 1; col <= 18; ++col)
        {
            expandedGameMusicLayout[9 * 20 + col] = gameMusicLayout[7 * 20 + col];
        }

        // Replace the copied A with th3 C character
        expandedGameMusicLayout[7 * 20 + 3] = gameMusicLayout[14 * 20 + 3];

        // Clear the copied B-TYPE text for the EXTRAS text
        for (int col = 11; col <= 16; ++col)
        {
            expandedGameMusicLayout[7 * 20 + col] = gameMusicLayout[5 * 20 + 9];
        }

        // Reuse the OG ROM outer frame and background
        extrasLayout = gameMusicLayout;

        const uint8_t dots = gameMusicLayout[1 * 20 + 1];
        const uint8_t blank = gameMusicLayout[3 * 20 + 9];

        // Remove the OG menu labels and boxes
        for (int row = 1; row <= 16; ++row)
        {
            for (int col = 1; col <= 18; ++col)
            {
                extrasLayout[row * 20 + col] = dots;
            }
        }

        // OG border tiles
        const uint8_t topLeft = gameMusicLayout[4 * 20 + 2];
        const uint8_t topMiddle = gameMusicLayout[4 * 20 + 3];
        const uint8_t topRight = gameMusicLayout[4 * 20 + 17];

        const uint8_t sideLeft = gameMusicLayout[5 * 20 + 2];
        const uint8_t sideRight = gameMusicLayout[5 * 20 + 17];
        
        const uint8_t bottomLeft = gameMusicLayout[6 * 20 + 2];
        const uint8_t bottomMiddle = gameMusicLayout[6 * 20 + 3];
        const uint8_t bottomRight = gameMusicLayout[6 * 20 + 17];

        // Construct decorative panels from those ROM tiles
        auto addExtrasPanel = [&](int left, int top,
            int right, int bottom)
            {
                for (int col = left; col <= right; ++col)
                {
                    extrasLayout[top * 20 + col] =
                        (col == left) ? topLeft :
                        (col == right) ? topRight : topMiddle;

                    extrasLayout[bottom * 20 + col] =
                        (col == left) ? bottomLeft :
                        (col == right) ? bottomRight : bottomMiddle;
                }

                for (int row = top + 1; row < bottom; ++row)
                {
                    for (int col = left; col <= right; ++col)
                    {
                        extrasLayout[row * 20 + col] =
                            (col == left) ? sideLeft :
                            (col == right) ? sideRight : blank;
                    }
                }
            };

        addExtrasPanel(5, 1, 14, 3);    // Title
        addExtrasPanel(1, 4, 18, 8);    // Palette
        addExtrasPanel(1, 9, 18, 11);   // Fullscreen
        addExtrasPanel(1, 12, 18, 16);  // Gameplay options

        // EXTRAS PAGE 2

        // copy Page one
        extrasAudioLayout = extrasLayout;

        // clear the copied Page one
        for (int row = 4; row <= 16; ++row)
        {
            for (int col = 1; col <= 18; ++col)
            {
                extrasAudioLayout[row * 20 + col] = dots;
            }
        }

        // create new boxes using the same tiles from Page one
        auto addAudioPanel = [&](int left, int top,
            int right, int bottom)
            {
                for (int col = left; col <= right; ++col)
                {
                    extrasAudioLayout[top * 20 + col] =
                        (col == left) ? topLeft :
                        (col == right) ? topRight : topMiddle;

                    extrasAudioLayout[bottom * 20 + col] =
                        (col == left) ? bottomLeft :
                        (col == right) ? bottomRight : bottomMiddle;
                }

                for (int row = top + 1; row < bottom; ++row)
                {
                    for (int col = left; col <= right; ++col)
                    {
                        extrasAudioLayout[row * 20 + col] =
                            (col == left) ? sideLeft :
                            (col == right) ? sideRight : blank;
                    }
                }
            };

        // audio panels :)
        addAudioPanel(1, 4, 18, 7);   // Master Volume
        addAudioPanel(1, 8, 18, 11);  // Music Volume
        addAudioPanel(1, 12, 18, 15); // SFX Volume

        aSelectLayout = extractLayoutAt(rom, LAYOUT_A_SELECT_OFFSET);
        bSelectLayout = extractLayoutAt(rom, LAYOUT_B_SELECT_OFFSET);
        titleTiles = BuildtitleTiles(rom);
        tiles = buildTiles(rom);
        layout = extractLayout(rom);
        pauseText = extractBytes(rom, PAUSE_TEXT_OFFSET, 80);
        gameOverText = extractBytes(rom, GAMEOVER_TEXT_OFFSET, 56);
        tryAgainText = extractBytes(rom, TRYAGAIN_TEXT_OFFSET, 48);
        musicRom = rom;
        romLoaded = true;
        SDL_SetWindowTitle(window, "WinBlocks");
    };

    tryLoadRom("tetris.gb");

    // ============================================================================
    // DISCORD RICH PRESENCE
    // Initialise the client and retain the session start timestamp.
    // ============================================================================

    const uint64_t DISCORD_APP_ID = 1560340105741471906ULL;

    auto discordClient = std::make_shared<discordpp::Client>();

    discordClient->SetApplicationId(DISCORD_APP_ID);

    const uint64_t discordStartedAtMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());

    Uint32 lastDiscordUpdateAt = 0;
    bool firstDiscordUpdate = true;

    bool viewer = false;

    // ============================================================================
    // GAMEPLAY STATE AND CLOCKS
    // Board, pieces, input repeat, entry delay, line-clear phases and SFX state.
    // ============================================================================

    for (int r = 0; r < FIELD_H; r++)
        for (int c = 0; c < FIELD_W; c++)
            field[r][c] = -1;

    Piece piece = { 0, 0, 3, -1 };

    Uint32 lastDrop = SDL_GetTicks();
    Uint32 dropMs = dropMsForLevel(0);
    Uint32 lastSoftDrop = 0;
    int heldDir = 0;
    Uint32 repeatAt = 0;
    int softRows = 0;
    bool needFreshDown = false;

    enum Mode { MODE_PLAYING, MODE_PAUSED, MODE_GAME_OVER_FILL, MODE_GAME_OVER_TEXT };
    Mode mode = MODE_PLAYING;
    Uint32 modeStart = 0;
    Uint32 gameOverRevealStart = 0;
    Uint32 pausedAt = 0;

    PulseChannel savedPulse1;
    PulseChannel savedPulse2;
    WaveChannel savedWave;
    NoiseChannel savedNoise;

    int pauseSfxStep = 4;
    Uint32 nextPauseSfxAt = 0;

    int gameOverWaveFrame = 30;
    uint8_t gameOverWavePitch = 0xD0;

    bool nextHidden = false;
    bool musicMuted = false;
    
    // for the hold system
    int heldPieceType = -1;
    bool holdUsedThisPiece = false;

    int entryPhase = 0;
    int stuckAtSpawnCount = 0;

    Uint32 entryStart = 0;

    bool clearing = false;
    Uint32 clearStart = 0;

    const Uint32 FLASH_MS = 167;
    const Uint32 SHIFT_DELAY_MS = 218;
    const Uint32 ROW_REVEAL_MS = 17;
    const Uint32 SHIFT_START_MS = 6 * FLASH_MS + SHIFT_DELAY_MS;
    const Uint32 CLEAR_MS = SHIFT_START_MS + FIELD_H * ROW_REVEAL_MS;

    int shiftedField[FIELD_H][FIELD_W]{};

    int lineSfxType = 0;
    int lineSfxStep = 0;
    Uint32 lineSfxStart = 0;

    bool tetrisWaveActive = false;
    Uint32 tetrisWaveStart = 0;
    int tetrisWaveFrame = 0;

    uint8_t tetrisWaveLow = 0x9D;

    int tetrisWaveDirection = 1;

    PulseChannel savedTetrisPulse2;
    WaveChannel savedTetrisWave;
    NoiseChannel savedTetrisNoise;

    hiddenType = 0;
    int nextType = 0;
    heldPieceType = -1;
    holdUsedThisPiece = false;

    // ============================================================================
    // GAME STATE TRANSITIONS
    // Piece spawning, game-over setup, restart and leaderboard transitions.
    // ============================================================================

    auto spawn = [&]() {
        piece = { nextType, 0, 3, -1 };
        nextType = advancePieces(piece.type);
        return !collides(piece);
    };
    spawn(); spawn(); spawn();

    auto beginGameOver = [&]() {

        stopRomSong();

        tetrisWaveActive = false;

        mode = MODE_GAME_OVER_FILL;
        modeStart = SDL_GetTicks();

        gameOverWaveFrame = 0;

        uint64_t gbCycles = static_cast<uint64_t>(static_cast<double>(SDL_GetPerformanceCounter()) * 4194304.0 / SDL_GetPerformanceFrequency());

        uint8_t div = static_cast<uint8_t>(gbCycles >> 8);

        gameOverWavePitch = static_cast<uint8_t>(0xD0 + (div & 0x1F));

        wave.writeNR30(0x00);

        if (musicRom.size() == ROM_SIZE)
        {
            std::memcpy(wave.waveRAM, musicRom.data() + 0x6ED9, 16);
        }

        wave.writeNR30(0x80);
        wave.writeNR31(0x3A);
        wave.writeNR32(0x20);
        wave.writeNR33(0x60);
        wave.writeNR34(0xC6);
    };

    auto restartGame = [&]() {
        musicMuted = false;
        for (int r = 0; r < FIELD_H; r++)
            for (int c = 0; c < FIELD_W; c++)
                field[r][c] = -1;
        resetStats();
        dropMs = dropMsForLevel(0);
        clearing = false;
        entryPhase = 0;
        tetrisWaveActive = false;
        stuckAtSpawnCount = 0;
        lastDrop = SDL_GetTicks();

        heldDir = 0;
        repeatAt = 0;

        softRows = 0;
        needFreshDown = false;
        lastSoftDrop = lastDrop;

        mode = MODE_PLAYING;

        gamePlayMs = 0;
        hiddenType = 0;
        nextType = 0;

        heldPieceType = -1;
        holdUsedThisPiece = false;

        spawn(); spawn(); spawn();
    };

    auto openScoreboard = [&]() {

        stopRomSong();

        pendingRank = -1;

        for (int i = 0; i < 3; i++)
        {
            if (score > 0 && score > highScores[playedLevel][i].score)
            {
                pendingRank = i;
                break;
            }
        }
        if (pendingRank >= 0)
        {
            for (int i = 2; i > pendingRank; i--)
            {
                highScores[playedLevel][i] = highScores[playedLevel][i - 1];
            }

            highScores[playedLevel][pendingRank] = { "------", score };

            namePosition = 0;
        }
        if (pendingRank >= 0)
        {
            startRomSong(1);
            screen = Screen::SCOREBOARD;
        }
        else
        {
            resumeChosenMusic();
            screen = Screen::A_LEVEL;
        }
    };

    auto landIfResting = [&]() {

        if (clearing)
            return;

        if (piece.x == 3 && piece.y == -1)
        {
            if (stuckAtSpawnCount != 0)
            {
                beginGameOver();
                return;
            }

            stuckAtSpawnCount = 1;
        }

        noiseSfxUntil = SDL_GetTicks() + 300;

        noise.writeNR41(0x00);
        noise.writeNR42(0xD1);
        noise.writeNR43(0x45);
        noise.writeNR44(0x80);

        score += softRows;
        if (score > 999999)
        {
            score = 999999;
        }
        softRows = 0;
        needFreshDown = true;

        lockPiece(piece);

        int fullRows = 0;

        for (int r = 0; r < FIELD_H; r++)
        {
            if (rowIsFull(r))
                fullRows++;
        }

        if (fullRows > 0)
        {
            int destinationRow = FIELD_H - 1;

            for (int sourceRow = FIELD_H - 1;
                sourceRow >= 0;
                sourceRow--)
            {
                if (rowIsFull(sourceRow))
                    continue;

                for (int c = 0; c < FIELD_W; c++)
                {
                    shiftedField[destinationRow][c] =
                    field[sourceRow][c];
                }

                destinationRow--;
            }

            while (destinationRow >= 0)
            {
                for (int c = 0; c < FIELD_W; c++)
                {
                    shiftedField[destinationRow][c] = -1;
                }

                destinationRow--;
            }

            clearing = true;
            clearStart = SDL_GetTicks();

            lineSfxType = fullRows;
            lineSfxStep = 0;
            lineSfxStart = clearStart;

            rotationSfxUntil = clearStart +
            (fullRows == 4 ? 420 : 1150);

            if (fullRows == 4)
            {

                playLinePulse(
                    0x00, 0x80, 0xE1, 0xC1, 0x87
                    );
            }
            else
            {

                playLinePulse(
                    0x3E, 0x80, 0xE3, 0x00, 0xC4
                    );
            }
        }
        else
        {

            entryPhase = 1;
            entryStart = SDL_GetTicks();
        }
    };

    bool running = true;

    // ============================================================================
    // MAIN LOOP
    // Process events, advance state, synthesize sound and render each frame.
    // ============================================================================

    Uint32 previousFrameTick = SDL_GetTicks();
    Uint32 lastStatisticSave = previousFrameTick;

    while (running)
    {

        // ============================================================================
        // INPUT AND WINDOW EVENTS
        // Keyboard, controller, window, file-drop and menu interactions.
        // ============================================================================

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_CONTROLLERDEVICEADDED)
            {
                if (!controller && SDL_IsGameController(event.cdevice.which))
                {
                    controller = SDL_GameControllerOpen(event.cdevice.which);

                    if (controller)
                    {
                        controllerId = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller)
                            );

                        SDL_Log("Controller connected: %s", SDL_GameControllerName(controller));
                    }
                }
            }
            else if (event.type == SDL_CONTROLLERDEVICEREMOVED)
            {
                if (controller && event.cdevice.which == controllerId)
                {
                    SDL_GameControllerClose(controller);

                    controller = nullptr;
                    controllerId = -1;
                    leftTriggerHeld = false;
                    rightTriggerHeld = false;

                    SDL_Log("Controller Die Bro?");
                }
            }
            else if (event.type == SDL_DROPFILE)
            {
                tryLoadRom(event.drop.file);
                SDL_free(event.drop.file);
            }

            // LT/RT Palette Swaps
            else if (event.type == SDL_CONTROLLERAXISMOTION)
            {
                if (!controller || event.caxis.which != controllerId)
                {
                    continue;
                }

                const Sint16 TRIGGER_PRESS = 16000;
                const Sint16 TRIGGER_RELEASE = 8000;

                // LT - Last Palette
                if (event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT)
                {
                    if (!leftTriggerHeld && event.caxis.value >= TRIGGER_PRESS)
                    {
                        leftTriggerHeld = true;
                        cyclePalette(-1);
                    }
                    else if (event.caxis.value <= TRIGGER_RELEASE)
                    {
                        leftTriggerHeld = false;
                    }
                }

                // RT - Next Palette 
                else if (event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
                {
                    if (!rightTriggerHeld && event.caxis.value >= TRIGGER_PRESS)
                    {
                        rightTriggerHeld = true;
                        cyclePalette(1);
                    }
                    else if (event.caxis.value <= TRIGGER_RELEASE)
                    {
                        rightTriggerHeld = false;
                    }
                }
            }
            else if (event.type == SDL_KEYDOWN || event.type == SDL_CONTROLLERBUTTONDOWN)
            {

                SDL_Keycode key = SDLK_UNKNOWN;

                if (event.type == SDL_KEYDOWN)
                {
                    if (event.key.repeat) continue;

                    key = event.key.keysym.sym;
                }

                else
                {
                    if (!controller || event.cbutton.which != controllerId) continue;

                    switch (event.cbutton.button)
                    {
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                        key = SDLK_LEFT;
                        break;

                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                        key = SDLK_RIGHT;
                        break;

                    case SDL_CONTROLLER_BUTTON_DPAD_UP:
                        key = SDLK_UP;
                        break;

                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                        key = SDLK_DOWN;
                        break;

                    case SDL_CONTROLLER_BUTTON_A:
                        key = (screen == Screen::IN_GAME ||
                            screen == Screen::SCOREBOARD)
                        ? SDLK_x : SDLK_RETURN;
                        break;

                    case SDL_CONTROLLER_BUTTON_B:
                        key = SDLK_z;
                        break;

                    case SDL_CONTROLLER_BUTTON_START:
                        key = (screen == Screen::IN_GAME)
                        ? SDLK_RETURN : SDLK_RETURN;
                        break;

                    case SDL_CONTROLLER_BUTTON_Y:
                        key = SDLK_F2;
                        break;

                    // Fullscreen
                    case SDL_CONTROLLER_BUTTON_BACK:
                        key = SDLK_F11;
                        break;

                    // Disable Next Piece Previewer  
                    case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
                        key = SDLK_BACKSPACE;
                        break;

                    // Toggle Music Ingame
                    case SDL_CONTROLLER_BUTTON_LEFTSTICK:
                        key = SDLK_F6;
                        break;

                    // LB for Extras Page one
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                        if (screen == Screen::SETTINGS)
                            key = SDLK_PAGEUP;
                        else
                        continue;
                        break;

                    // RB for Page two, ingame RB is used for piece holding    
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                        if (screen == Screen::SETTINGS)
                            key = SDLK_PAGEDOWN;
                        else
                            key = SDLK_c;
                        break;

                    default:
                        continue;
                    }
                }

                // F6 - disable music
                if (key == SDLK_F6)
                {
                    if (screen == Screen::IN_GAME && mode == MODE_PLAYING && !viewer && selectedMusic != 3)
                    {
                        musicMuted = !musicMuted;

                        if (musicMuted)
                        {
                            stopRomSong();
                        }
                        else
                        {
                            resumeChosenMusic();
                        }

                        if (audioDevice != 0)
                        {
                            SDL_ClearQueuedAudio(audioDevice);
                        }
                        SDL_Log("Gameplay music: %s", musicMuted ? "OFF" : "ON");
                    }

                    continue;
                }

                if (key == SDLK_F11)
                {
                    toggleFullscreen();
                    continue;
                }

                // NEW for v1.1
                if (key == SDLK_F2 && !viewer && (screen == Screen::GAME_MUSIC || screen == Screen::SETTINGS))
                {
                    if (screen == Screen::SETTINGS)
                    {
                        screen = settingsReturnScreen;
                    }
                    else if (screen == Screen::TITLE ||
                        screen == Screen::GAME_MUSIC ||
                        screen == Screen::A_LEVEL ||
                        screen == Screen::B_LEVEL ||
                        screen == Screen::B_HIGH)
                    {
                        settingsReturnScreen = screen;
                        settingsSelection = 0;
                        settingsPage = 0;
                        screen = Screen::SETTINGS;
                    }
                    continue;
                }
                
                if (screen == Screen::IN_GAME &&
                    mode == MODE_PLAYING && !viewer &&
                    !clearing && entryPhase == 0)
                {
                    // HARD DROP
                    if (hardDropEnabled && key == SDLK_UP)
                    {
                        while (tryMove(piece, 0, 1))
                        {
                        }

                        landIfResting();
                        continue;
                    }

                    // HOLD PIECE
                    if (holdEnabled && key == SDLK_c)
                    {
                        if (!holdUsedThisPiece)
                        {
                            holdUsedThisPiece = true;

                            int currentType = piece.type;

                            if (heldPieceType < 0)
                            {
                                heldPieceType = currentType;

                                if (!spawn())
                                    beginGameOver();
                            }
                            else
                            {
                                piece = { heldPieceType, 0, 3, -1 };
                                heldPieceType = currentType;

                                if (collides(piece))
                                    beginGameOver();
                            }

                            lastDrop = SDL_GetTicks();
                            lastSoftDrop = lastDrop;
                            softRows = 0;
                            heldDir = 0;
                            repeatAt = 0;
                            needFreshDown = true;
                        }

                        continue;
                    }
                }

                // F3 allows instant Palette swaps, even ingame 
                // F3 = Next Palette, F3 + Shift for the Previous
                if (key == SDLK_F3)
                {
                    int direction = (SDL_GetModState() & KMOD_SHIFT) ? -1 : 1;

                    cyclePalette(direction);
                    continue;
                }

                if (screen == Screen::SETTINGS)
                {
                    // return to last menu
                    if (key == SDLK_ESCAPE || key == SDLK_z || key == SDLK_BACKSPACE)
                    {
                        screen = settingsReturnScreen;
                    }

                    // swap between the extra pages
                    else if (key == SDLK_TAB || key == SDLK_PAGEUP || key == SDLK_PAGEDOWN)
                    {
                        if (key == SDLK_PAGEUP)
                            settingsPage = 0;
                        else if (key == SDLK_PAGEDOWN)
                            settingsPage = 1;
                        else
                            settingsPage = 1 - settingsPage;

                        // start on the first option of each page
                        settingsSelection = 0;
                        playMenuSound(false);
                    }

                    // move selection cursor up
                    else if (key == SDLK_UP)
                    {
                        if (settingsSelection > 0)
                            --settingsSelection;
                    }

                    // move the selection cursor down
                    else if (key == SDLK_DOWN)
                    {
                        int last = (settingsPage == 0) ? 4 : 2;

                        if (settingsSelection < last)
                            ++settingsSelection;
                    }

                    // change the highlighted setting
                    else if (key == SDLK_LEFT ||
                        key == SDLK_RIGHT ||
                        key == SDLK_RETURN ||
                        key == SDLK_SPACE ||
                        key == SDLK_x)
                    {
                        int direction =
                            (key == SDLK_LEFT) ? -1 : 1;

                        if (settingsPage == 0)
                        {
                            // extras Page one
                            if (settingsSelection == 0)
                            {
                                cyclePalette(direction);
                            }
                            else if (settingsSelection == 1)
                            {
                                toggleFullscreen();
                            }
                            else
                            {
                                if (settingsSelection == 2)
                                    ghostEnabled = !ghostEnabled;
                                else if (settingsSelection == 3)
                                    hardDropEnabled = !hardDropEnabled;
                                else if (settingsSelection == 4)
                                    holdEnabled = !holdEnabled;

                                savePreferences();
                            }
                        }
                        else
                        {
                            // extras Page two
                            int* target =
                                (settingsSelection == 0) ? &masterVolume :
                                (settingsSelection == 1) ? &musicVolume :
                                &sfxVolume;

                            *target = std::clamp(
                                *target + direction * 10,
                                0,
                                100
                            );

                            savePreferences();
                            playMenuSound(false);
                        }
                    }

                    continue;
                }

                if (key == SDLK_DOWN && !viewer && screen == Screen::IN_GAME)
                {
                    const Uint8* k = SDL_GetKeyboardState(nullptr);

                    bool leftHeld = k[SDL_SCANCODE_LEFT] || padHeld(SDL_CONTROLLER_BUTTON_DPAD_LEFT);

                    bool rightHeld = k[SDL_SCANCODE_RIGHT] || padHeld(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

                    if (!leftHeld && !rightHeld)
                    {
                        needFreshDown = false;
                    }
                }

                if (!clearing)
                {
                    if (screen == Screen::IN_GAME && mode == MODE_PLAYING && !viewer && entryPhase == 0)
                    {
                        bool rotated = false;

                        if (key == SDLK_z || key == SDLK_SPACE) rotated = tryRotate(piece, 1);

                        if (key == SDLK_x) rotated = tryRotate(piece, -1);

                        if (rotated)
                        {
                            rotationSfxUntil = SDL_GetTicks() + 300;
                            menuMoveSecondNotePending = false;
                            pulse1.nr10 = 0x3B;
                            pulse1.nr12 = 0xB2;

                            pulse1.writeNR11(0x80);
                            pulse1.writeNR13(0x87);
                            pulse1.writeNR14(0x87);
                        }
                    }
                }

                if (!viewer)
                {
                    if (key == SDLK_RETURN && screen == Screen::IN_GAME)
                    {
                        if (mode == MODE_PLAYING)
                        {
                            pausedAt = SDL_GetTicks();
                            savedPulse1 = pulse1;
                            savedPulse2 = pulse2;
                            savedWave = wave;
                            savedNoise = noise;

                            mode = MODE_PAUSED;

                            pulse1.enabled = false;
                            pulse2.enabled = false;
                            wave.enabled = false;
                            noise.enabled = false;

                            pauseSfxStep = 0;
                            nextPauseSfxAt = pausedAt;

                            menuMoveSecondNotePending = false;

                            if (audioDevice != 0) SDL_ClearQueuedAudio(audioDevice);
                        }
                        else if (mode == MODE_PAUSED)
                        {
                            Uint32 d = SDL_GetTicks() - pausedAt;

                            lastDrop += d;
                            lastSoftDrop += d;
                            clearStart += d;
                            repeatAt += d;

                            if (entryPhase != 0)
                                entryStart += d;

                            if (tetrisWaveActive)
                                tetrisWaveStart += d;

                            nextMusicNoteAt += d;
                            nextMusic1NoteAt += d;
                            nextMusic3NoteAt += d;
                            nextMusic4NoteAt += d;

                            if (rotationSfxUntil != 0) rotationSfxUntil += d;

                            if (noiseSfxUntil != 0) noiseSfxUntil += d;

                            if (lineSfxType != 0) lineSfxStart += d;

                            pulse1 = savedPulse1;
                            pulse2 = savedPulse2;
                            wave = savedWave;
                            noise = savedNoise;

                            pauseSfxStep = 4;
                            mode = MODE_PLAYING;

                            if (audioDevice != 0) SDL_ClearQueuedAudio(audioDevice);
                        }
                        else if (mode == MODE_GAME_OVER_TEXT && screen == Screen::IN_GAME)
                        {
                            openScoreboard();

                            continue;
                        }
                    }
                    if (key == SDLK_x && mode == MODE_GAME_OVER_TEXT && screen == Screen::IN_GAME)
                    {
                        openScoreboard();

                        continue;
                    }

                    if (key == SDLK_BACKSPACE && screen == Screen::IN_GAME && mode == MODE_PLAYING && !viewer)
                    {
                        nextHidden = !nextHidden;
                    }

                    if (key == SDLK_z || key == SDLK_ESCAPE)
                    {
                        if (screen == Screen::A_LEVEL)
                        {
                            screen = Screen::GAME_MUSIC;

                            musicMenuFocus = false;

                            continue;
                        }
                        if (screen == Screen::GAME_MUSIC)
                        {
                            if (musicMenuFocus)
                            {
                                musicMenuFocus = false;
                            }

                            continue;
                        }
                    }

                    if (screen == Screen::TITLE)
                    {
                        if (key == SDLK_LEFT) numPlayers = 0;
                        if (key == SDLK_RIGHT) numPlayers = 1;
                        if (key == SDLK_RETURN && numPlayers == 0)
                        {
                            screen = Screen::GAME_MUSIC;
                            musicMenuFocus = false;

                            if (selectedMusic == 3)
                            {
                                stopRomSong();
                            }
                            else
                            {
                                startRomSong(5 + selectedMusic);
                            }
                        }
                    }
                    else if (screen == Screen::GAME_MUSIC)
                    {
                        bool selectionChanged = false;
                        bool focusChanged = false;

                        if (!musicMenuFocus)
                        {
                            // A-TYPE   B-TYPE
                            // C-TYPE   EXTRAS
                            int previousType = selectedGameType;

                            if (key == SDLK_LEFT && selectedGameType % 2 == 1)
                                selectedGameType--;

                            else if (key == SDLK_RIGHT && selectedGameType % 2 == 0)
                                selectedGameType++;

                            else if (key == SDLK_UP && selectedGameType >= 2)
                                selectedGameType -= 2;

                            else if (key == SDLK_DOWN)
                            {
                                if (selectedGameType < 2)
                                {
                                    selectedGameType += 2;
                                }
                                else
                                {
                                    musicMenuFocus = true;
                                    focusChanged = true;
                                }
                            }

                            if (selectedGameType != previousType)
                            {
                                selectionChanged = true;
                            }
                        }
                        else
                        {
                            int previousMusic = selectedMusic;

                            if (key == SDLK_LEFT && selectedMusic % 2 == 1) selectedMusic--;

                            else if (key == SDLK_RIGHT && selectedMusic % 2 == 0) selectedMusic++;

                            else if (key == SDLK_DOWN && selectedMusic < 2) selectedMusic += 2;

                            else if (key == SDLK_UP)
                            {
                                if (selectedMusic >= 2) selectedMusic -= 2;
                                
                                else
                                {
                                    musicMenuFocus = false;
                                    // Return to C-TYPE or EXTRAS
                                    selectedGameType = 2 + selectedMusic % 2;
                                    focusChanged = true;
                                }
                            }

                            if (selectedMusic != previousMusic)
                            {
                                selectionChanged = true;

                                if (selectedMusic == 3)
                                {
                                    stopRomSong();
                                }
                                else
                                {
                                    startRomSong(5 + selectedMusic);
                                }
                            }
                        }

                        // fix: play the lil bleep anytime the selection changes
                        if (selectionChanged || focusChanged)
                        {
                            playMenuSound(false);
                        }

                        if (key == SDLK_RETURN)
                        {
                            if (musicMenuFocus)
                            {
                                // Confirm music and return to type selection
                                musicMenuFocus = false;

                                selectedGameType = selectedMusic % 2;

                                playMenuSound(true);
                            }
                            else if (selectedGameType == 0)
                            {
                                playMenuSound(true);
                                screen = Screen::A_LEVEL;
                            }
                            else if (selectedGameType == 1)
                            {
                                SDL_Log("B-TYPE got stuck in traffic");
                            }
                            else if (selectedGameType == 2)
                            {
                                SDL_Log("C-TYPE has left the building");
                            }
                            else if (selectedGameType == 3)
                            {
                                playMenuSound(true);

                                settingsReturnScreen = Screen::GAME_MUSIC;
                                settingsSelection = 0;
                                settingsPage = 0;

                                screen = Screen::SETTINGS;
                            }
                        }
                    }
                    else if (screen == Screen::A_LEVEL)
                    {
                        if (key == SDLK_RETURN)
                        {
                            playMenuSound(true);
                            restartGame();
                            playedLevel = aLevel;
                            level = aLevel;
                            dropMs = dropMsForLevel(level);
                            screen = Screen::IN_GAME;
                        }
                        int previousLevel = aLevel;

                        if (key == SDLK_LEFT && aLevel > 0)
                        {
                            aLevel--;
                        }
                        else if (key == SDLK_RIGHT && aLevel < 9)
                        {
                            aLevel++;
                        }
                        else if (key == SDLK_UP && aLevel >= 5)
                        {
                            aLevel -= 5;
                        }
                        else if (key == SDLK_DOWN && aLevel < 5)
                        {
                            aLevel += 5;
                        }
                        if (aLevel != previousLevel)
                        {
                            playMenuSound(false);
                        }
                    }
                    else if (screen == Screen::SCOREBOARD)
                    {
                        if (pendingRank >= 0)
                        {
                            std::string& name = highScores[playedLevel][pendingRank].name;

                            if (key == SDLK_UP || key == SDLK_DOWN)
                            {
                                static const std::string SCORE_CHARACTERS =
                                    "-ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

                                size_t found = SCORE_CHARACTERS.find(name[namePosition]);

                                int index = (found == std::string::npos)
                                    ? 0 : static_cast<int>(found);

                                int step = (key == SDLK_UP) ? 1 : -1;
                                int count = static_cast<int>(SCORE_CHARACTERS.size());

                                index = (index + step + count) % count;

                                name[namePosition] = SCORE_CHARACTERS[index];
                            }
                            else if (key == SDLK_x || key == SDLK_SPACE)
                            {
                                if (namePosition < HIGH_SCORE_NAME_LENGTH - 1)
                                {
                                    namePosition++;
                                }
                                else
                                {
                                    pendingRank = -1;
                                    resumeChosenMusic();
                                    screen = Screen::A_LEVEL;
                                }

                                saveHighScores(highScores);
                            }
                            else if (key == SDLK_z)
                            {
                                if (namePosition > 0)
                                    namePosition--;
                            }
                            else if (key == SDLK_RETURN)
                            {
                                pendingRank = -1;

                                saveHighScores(highScores);
                                resumeChosenMusic();

                                screen = Screen::A_LEVEL;
                            }
                        }
                        else if (key == SDLK_RETURN)
                        {
                            screen = Screen::A_LEVEL;
                        }
                    }
                }
            }
        }

        // ============================================================================
        // RICH PRESENCE UPDATES
        // Update Discord activity using the existing game state.
        // ============================================================================

        discordpp::RunCallbacks();

        // playtime tracking
        const Uint32 frameNow = SDL_GetTicks(); const Uint32 frameElapsed = frameNow - previousFrameTick;

        previousFrameTick = frameNow;

        // for the rainbow palette
        if (paletteIndex == PALETTE_COUNT - 1)
        {
            // time to complete a cycle
            const Uint32 RAINBOW_CYCLE_MS = 25000;

            // hue will move from 0 to 360  
            float hue = (frameNow % RAINBOW_CYCLE_MS) * (360.0f / RAINBOW_CYCLE_MS);

            // the saturation for each shade
            const float saturation[4] =
            {
                0.25f, 0.50f, 0.80f, 0.92f
            };

            // the brightness of each shade
            const float brightness[4] =
            {
                1.00f, 0.78f, 0.46f, 0.19f
            };

            // update the four shades
            for (int i = 0; i < 4; ++i)
            {
                PALETTE[i] = hueToColor(hue, saturation[i], brightness[i]);
            }
        }

        // only count during gameplay
        if (romLoaded && screen == Screen::IN_GAME && mode == MODE_PLAYING && !viewer)
        {
            // stop counting if paused
            const Uint32 activeElapsed = std::min<Uint32>(frameElapsed, 1000);

            sessionPlayMs += activeElapsed;
            gamePlayMs += activeElapsed;
        }

        // write the total every 15 seconds
        if (frameNow - lastStatisticSave >= 15000)
        {
            savePreferences();
            lastStatisticSave = frameNow;
        }


        Uint32 discordNow = SDL_GetTicks();

        if (firstDiscordUpdate || discordNow - lastDiscordUpdateAt >= 3000)
        {
            firstDiscordUpdate = false;
            lastDiscordUpdateAt = discordNow;

            std::string details;
            std::string state;

            switch (screen)
            {
            case Screen::COPYRIGHT:
                details = "Starting WinBlocks";
                state = "Loading";
                break;

            case Screen::TITLE:
                details = "At the Title Screen";
                state = "Main Menu";
                break;

            case Screen::SETTINGS:
                details = "Tinkering...";
                state = "Settings";
                break;

            case Screen::GAME_MUSIC:
                details = "Choosing Game Settings";
                state = "Selecting Music and Game Type";
                break;

            case Screen::A_LEVEL:
                details = "Selecting a Level";
                state = "A-Type | Level " +
                std::to_string(aLevel);
                break;

            case Screen::B_LEVEL:
            case Screen::B_HIGH:
                details = "Selecting B-Type Settings";
                state = "Game Setup";
                break;

            case Screen::IN_GAME:
                if (mode == MODE_PAUSED)
                {
                    details = "Paused | A-Type";
                }
                else if (mode == MODE_GAME_OVER_FILL || mode == MODE_GAME_OVER_TEXT)
                {
                    details = "Game Over | A-Type";
                }
                else
                {
                    details = "Playing A-Type | Level " + std::to_string(level);
                }

                state = "Score: " + std::to_string(score) + " | Lines: " + std::to_string(lines);
                break;

            case Screen::SCOREBOARD: details = "Viewing High Scores"; state = (pendingRank >= 0) ? "Entering a New High Score" : "High Score Table";
                break;
            }

            discordpp::Activity activity;
            activity.SetType(discordpp::ActivityTypes::Playing);
            activity.SetDetails(details);
            activity.SetState(state);

            discordpp::ActivityAssets assets;
            assets.SetLargeImage("winblocks_icon");
            assets.SetLargeText("WinBlocks");
            activity.SetAssets(assets);

            discordpp::ActivityTimestamps timestamps;
            timestamps.SetStart(discordStartedAtMs);
            activity.SetTimestamps(timestamps);

            discordClient->UpdateRichPresence(activity, [](const discordpp::ClientResult& result)
                {
                    if (!result.Successful())
                    {
                        SDL_Log("Discord Rich Presence update failed: %s", result.Error().c_str());
                }
            }
                );
        }

        // ============================================================================
        // PIECE ENTRY AND GAMEPLAY UPDATE
        // Apply spawn timing, gameplay controls, soft drop and gravity.
        // ============================================================================

        if (screen == Screen::IN_GAME && mode == MODE_PLAYING && entryPhase != 0)
        {
            Uint32 elapsed = SDL_GetTicks() - entryStart;

            if (entryPhase == 1 && elapsed >= 16)
            {
                entryPhase = 2;

                spawn();
                holdUsedThisPiece = false;
            }

            if (entryPhase == 2 && elapsed >= 33)
            {
                entryPhase = 0;

                lastDrop = SDL_GetTicks();
                lastSoftDrop = lastDrop;
            }
        }

        bool active = screen == Screen::IN_GAME &&
        mode == MODE_PLAYING &&
        entryPhase == 0;

        // ============================================================================
        // GAME-OVER SOUND AND TRANSITION
        // Advance the Game Over wave effect and screen state.
        // ============================================================================

        if (mode == MODE_GAME_OVER_FILL && gameOverWaveFrame < 30)
        {
            int targetFrame = static_cast<int>((SDL_GetTicks() - modeStart) * 59.73 / 1000.0);

            while (gameOverWaveFrame < 30 && gameOverWaveFrame < targetFrame)
            {
                gameOverWaveFrame++;

                if (gameOverWaveFrame >= 30)
                {
                    wave.writeNR30(0x00);
                    wave.enabled = false;
                    break;
                }

                if (gameOverWaveFrame < 14)
                {
                    gameOverWavePitch = static_cast<uint8_t>(gameOverWavePitch + 2);
                }

                else
                {
                    gameOverWavePitch = static_cast<uint8_t>(gameOverWavePitch - 3);
                }

                uint64_t gbCycles = static_cast<uint64_t>(static_cast<double>(SDL_GetPerformanceCounter()) * 4194304.0 / SDL_GetPerformanceFrequency());
                uint8_t div = static_cast<uint8_t>(gbCycles >> 8);
                uint8_t frequencyLow = static_cast<uint8_t>((gameOverWavePitch & 0xF0) | (div & 0x0F));

                wave.writeNR33(frequencyLow);
            }
        }

        if (mode == MODE_GAME_OVER_FILL && SDL_GetTicks() - modeStart >= 1172)
        {
            gameOverWaveFrame = 30;
            wave.writeNR30(0x00);
            gameOverRevealStart = SDL_GetTicks();

            mode = MODE_GAME_OVER_TEXT;
            startRomSong(4);
        }

        if (active && !clearing)
        {
            const Uint8* hk = SDL_GetKeyboardState(nullptr);

            bool right = hk[SDL_SCANCODE_RIGHT] || padHeld(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

            bool left = hk[SDL_SCANCODE_LEFT] || padHeld(SDL_CONTROLLER_BUTTON_DPAD_LEFT);

            int dir = right ? 1 : (left ? -1 : 0);
            Uint32 now = SDL_GetTicks();

            if (dir == 0)
            {
                heldDir = 0;
            }
            else if (dir != heldDir)
            {
                heldDir = dir;
                repeatAt = tryMove(piece, dir, 0) ? now + 385 : now;
            }
            else if (now >= repeatAt)
            {
                repeatAt = tryMove(piece, dir, 0) ? now + 151 : now;
            }
        }
        const Uint8* keys = SDL_GetKeyboardState(nullptr);

        bool downHeld = keys[SDL_SCANCODE_DOWN] || padHeld(SDL_CONTROLLER_BUTTON_DPAD_DOWN);

        bool leftHeld = keys[SDL_SCANCODE_LEFT] || padHeld(SDL_CONTROLLER_BUTTON_DPAD_LEFT);

        bool rightHeld = keys[SDL_SCANCODE_RIGHT] || padHeld(SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

        bool downOnly = downHeld && !leftHeld && !rightHeld;
        bool softDropping = active && !clearing && downOnly && !needFreshDown;

        if (!downOnly) softRows = 0;

        if (softDropping && SDL_GetTicks() - lastSoftDrop >= 50)
        {
            lastSoftDrop = SDL_GetTicks();
            lastDrop = SDL_GetTicks();
            if (tryMove(piece, 0, 1)) softRows++;
            else landIfResting();
        }

        if (active && !clearing && !softDropping && SDL_GetTicks() - lastDrop >= dropMs)
        {
            lastDrop = SDL_GetTicks();
            if (!tryMove(piece, 0, 1)) landIfResting();
        }

        // ============================================================================
        // LINE-CLEAR PULSE SEQUENCES
        // Trigger the existing normal and four-line pulse SFX steps.
        // ============================================================================

        if (clearing && mode == MODE_PLAYING && lineSfxType != 0)
        {

            int effectFrame = static_cast<int>(
                (SDL_GetTicks() - lineSfxStart) * 59.73 / 1000.0
                );

            if (lineSfxType == 4)
            {

                const int changeFrames[4] = { 4, 11, 15, 24 };

                while (lineSfxStep < 4 &&
                    effectFrame >= changeFrames[lineSfxStep])
                {

                    if (lineSfxStep == 3)
                    {
                        lineSfxType = 0;
                        rotationSfxUntil = 0;

                        if (musicRom.size() == ROM_SIZE)
                        {
                            savedTetrisPulse2 = pulse2;
                            savedTetrisWave = wave;
                            savedTetrisNoise = noise;

                            pulse1.enabled = false;
                            pulse2.enabled = false;
                            noise.enabled = false;

                            tetrisWaveActive = true;
                            tetrisWaveStart = SDL_GetTicks();
                            tetrisWaveFrame = 0;

                            tetrisWaveLow = 0x9D;
                            tetrisWaveDirection = 1;

                            wave.writeNR30(0x00);

                            std::memcpy(wave.waveRAM, musicRom.data() + 0x6EA9, 16);

                            wave.writeNR31(0x80);
                            wave.writeNR32(0x20);
                            wave.writeNR33(0x9D);
                            wave.writeNR30(0x80);
                            wave.writeNR34(0x87);
                        }
                        else
                        {
                            playMusic1Note();
                        }
                    }
                    else
                    {

                        uint8_t frequencyLow =
                        (lineSfxStep == 1) ? 0xC1 : 0xAC;

                        playLinePulse(
                            0x00, 0x80, 0xE1,
                            frequencyLow, 0x87
                            );
                    }

                    lineSfxStep++;
                }
            }
            else
            {

                static const uint8_t envelopes[10] =
                {
                    0x93, 0x83, 0x83, 0x73, 0x63,
                    0x53, 0x43, 0x33, 0x23, 0x13
                };

                static const uint8_t frequencies[10] =
                {
                    0x00, 0x23, 0x43, 0x63, 0x83,
                    0xA3, 0xC3, 0xD3, 0xE3, 0xFF
                };

                while (lineSfxStep <= 10 &&
                    effectFrame >= (lineSfxStep + 1) * 6)
                {
                    if (lineSfxStep == 10)
                    {

                        lineSfxType = 0;
                        rotationSfxUntil = 0;
                        playMusic1Note();
                    }
                    else
                    {
                        pulse1.nr12 = envelopes[lineSfxStep];

                        pulse1.writeNR13(
                            frequencies[lineSfxStep]
                            );

                        pulse1.writeNR14(0x86);
                    }

                    lineSfxStep++;
                }
            }
        }

        // ============================================================================
        // FOUR-LINE WAVE FINALE
        // Run the existing special wave effect and restore soundtrack channels.
        // ============================================================================

        if (tetrisWaveActive && screen == Screen::IN_GAME && mode == MODE_PLAYING)
        {
            int targetFrame = static_cast<int>((SDL_GetTicks() - tetrisWaveStart) * 59.73 / 1000.0);

            if (targetFrame > 32)
                targetFrame = 32;

            while (tetrisWaveFrame < targetFrame)
            {
                tetrisWaveFrame++;

                if (tetrisWaveFrame == 32)
                {
                    wave.writeNR30(0x00);

                    pulse2 = savedTetrisPulse2;
                    wave = savedTetrisWave;
                    noise = savedTetrisNoise;

                    double pausedMusicMs = static_cast<double>(SDL_GetTicks() - tetrisWaveStart);

                    nextMusic1NoteAt += pausedMusicMs;
                    nextMusicNoteAt += pausedMusicMs;
                    nextMusic3NoteAt += pausedMusicMs;
                    nextMusic4NoteAt += pausedMusicMs;

                    tetrisWaveActive = false;

                    playMusic1Note();
                    break;
                }

                if (tetrisWaveFrame == 9 ||
                    tetrisWaveFrame == 19 ||
                    tetrisWaveFrame == 23)
                {
                    uint8_t length = 0x80;

                    if (tetrisWaveFrame == 9)
                    {
                        length = 0xF8;
                        tetrisWaveLow = 0x98;
                        tetrisWaveDirection = 0;
                    }
                    else if (tetrisWaveFrame == 19)
                    {
                        length = 0xFB;
                        tetrisWaveLow = 0x96;
                        tetrisWaveDirection = 1;
                    }
                    else
                    {
                        length = 0xF6;
                        tetrisWaveLow = 0x95;
                        tetrisWaveDirection = -1;
                    }

                    wave.writeNR31(length);
                    wave.writeNR32(0x20);
                    wave.writeNR33(tetrisWaveLow);
                    wave.writeNR30(0x80);
                    wave.writeNR34(0x87);
                }
                else if (tetrisWaveDirection != 0)
                {
                    if (tetrisWaveDirection > 0)
                        tetrisWaveLow += 2;
                    else
                        tetrisWaveLow -= 2;

                    wave.writeNR33(tetrisWaveLow);
                }
            }
        }

        // ============================================================================
        // LINE REMOVAL AND SPAWNING
        // Complete the clear animation and begin the following piece entry.
        // ============================================================================

        if (clearing && mode == MODE_PLAYING &&
            SDL_GetTicks() - clearStart >= CLEAR_MS)
        {
            int removed = removeFullRows();
            addLines(removed);

            if (removed > 0)
            {
                Uint32 now = SDL_GetTicks();

                noise.writeNR41(0x00);
                noise.writeNR42(0xF1);
                noise.writeNR43(0x54);
                noise.writeNR44(0x80);
                noiseSfxUntil = now + 540;
            }

            dropMs = dropMsForLevel(level);
            clearing = false;
            lineSfxType = 0;

            entryPhase = 1;
            entryStart = SDL_GetTicks();
        }

        // ============================================================================
        // STARTUP AND MENU SFX TIMERS
        // Advance the boot chime, menu movement sound and pause jingle.
        // ============================================================================

        if (romLoaded && screen == Screen::COPYRIGHT && startupChimeStep < 3 && SDL_GetTicks() >= startupChimeAt)
        {
            if (startupChimeStep == 0)
            {
                pulse1.nr10 = 0x00;
                pulse1.writeNR11(0x80);
                pulse1.nr12 = 0xF3;

                pulse1.writeNR13(0x83);
                pulse1.writeNR14(0x87);

                startupChimeAt += 67;
            }
            else if (startupChimeStep == 1)
            {
                pulse1.writeNR13(0xC1);
                pulse1.writeNR14(0x87);

                startupChimeAt += 800;
            }
            else
            {
                pulse1.enabled = false;
            }

            startupChimeStep++;
        }

        if (menuMoveSecondNotePending && SDL_GetTicks() >= menuMoveSecondNoteAt)
        {
            menuMoveSecondNotePending = false;

            if (SDL_GetTicks() < rotationSfxUntil)
            {
                pulse1.nr10 = 0x00;
                pulse1.writeNR11(0xB5);
                pulse1.nr12 = 0x20;
                pulse1.writeNR13(0x40);
                pulse1.writeNR14(0xC7);
            }
        }

        if (screen == Screen::IN_GAME && mode == MODE_PAUSED && pauseSfxStep < 4 && SDL_GetTicks() >= nextPauseSfxAt)
        {
            pulse2.writeNR11(0xB2);
            pulse2.nr12 = 0xE3;

            uint8_t frequencyLow = (pauseSfxStep % 2 == 0) ? 0x83 : 0xC1;

            pulse2.writeNR13(frequencyLow);
            pulse2.writeNR14(0xC7);

            pauseSfxStep++;
            nextPauseSfxAt += 134;
        }

        // ============================================================================
        // COPYRIGHT SCREEN TIMING
        // Move from the initial screen to the title screen.
        // ============================================================================

        if (screen == Screen::COPYRIGHT)
        {
            Uint32 now = SDL_GetTicks();

            if (now - screenStart >= 8372)
            {
                screen = Screen::TITLE;
                screenStart = now;
                startRomSong(3);
            }
        }

        // ============================================================================
        // MUSIC CHANNEL 2
        // Decode and schedule the main pulse melody.
        // ============================================================================

        if (musicPreviewPlaying && mode != MODE_PAUSED && !tetrisWaveActive && SDL_GetTicks() >= nextMusicNoteAt)
        {
            bool scheduled = false;

            for (int attempt = 0; attempt < 32; attempt++)
            {
                if (musicCursor >= musicRom.size())
                {
                    musicPreviewPlaying = false;
                    pulse2.enabled = false;
                    break;
                }

                uint8_t command = musicRom[musicCursor++];

                if (command == 0x00)
                {
                    musicSectionPointer += 2;

                    uint16_t nextSection = readMusicWord(musicSectionPointer);

                    if (nextSection == 0xFFFF)
                    {
                        musicSectionPointer = readMusicWord(musicSectionPointer + 2);

                        nextSection = readMusicWord(musicSectionPointer);
                    }

                    if (nextSection == 0 || nextSection >= musicRom.size())
                    {
                        musicPreviewPlaying = false;
                        pulse2.enabled = false;
                        break;
                    }

                    musicCursor = nextSection;
                    continue;
                }

                if (command == 0x9D)
                {
                    if (musicCursor + 3 > musicRom.size())
                    {
                        musicPreviewPlaying = false;
                        pulse2.enabled = false;
                        break;
                    }

                    pulse2.nr12 = musicRom[musicCursor];
                    pulse2.writeNR11(musicRom[musicCursor + 2]);

                    musicCursor += 3;
                    continue;
                }

                if ((command & 0xF0) == 0xA0)
                {
                    int tempoIndex = command & 0x0F;

                    musicTempoFrames = musicRom[musicTempoTable + tempoIndex];

                    continue;
                }

                double durationMs = musicTempoFrames * 1000.0 / 59.73;
                if (durationMs < 1.0) durationMs = 1.0;

                if (command == 0x01)
                {
                    pulse2.enabled = false;
                    nextMusicNoteAt += durationMs;

                    scheduled = true;

                    break;
                }

                if (command >= 2 && command <= 0x90 && (command % 2) == 0)
                {
                    size_t frequencyOffset = 0x6E04 + command - 2;

                    if (frequencyOffset + 1 >= musicRom.size())
                    {
                        musicPreviewPlaying = false;
                        pulse2.enabled = false;
                        break;
                    }

                    uint16_t period = readMusicWord(frequencyOffset);

                    pulse2.writeNR13(static_cast<uint8_t>(period & 0xFF));
                    pulse2.writeNR14(static_cast<uint8_t>(0x80 | ((period >> 8) & 0x07)));

                    nextMusicNoteAt += durationMs;
                    scheduled = true;

                    break;
                }

                SDL_Log("Unsupported music command: 0x%02X", command);

                musicPreviewPlaying = false;
                pulse2.enabled = false;
                break;
            }

            if (musicPreviewPlaying && !scheduled)
            {
                SDL_Log("Music decoder command limit reached.");

                musicPreviewPlaying = false;
                pulse2.enabled = false;
            }
        }

        // ============================================================================
        // MUSIC CHANNEL 1
        // Decode and schedule the pulse accompaniment.
        // ============================================================================

        if (musicPreviewPlaying && mode != MODE_PAUSED && !tetrisWaveActive && SDL_GetTicks() >= nextMusic1NoteAt)
        {
            bool scheduled = false;
            Uint32 now = SDL_GetTicks();

            for (int attempt = 0; attempt < 32; attempt++)
            {
                if (music1Cursor >= musicRom.size())
                    break;

                uint8_t command = musicRom[music1Cursor++];

                if (command == 0x00)
                {
                    music1SectionPointer += 2;

                    if (music1SectionPointer + 1 >= musicRom.size())
                        break;

                    uint16_t section = readMusicWord(music1SectionPointer);

                    if (section == 0xFFFF)
                    {
                        if (music1SectionPointer + 3 >= musicRom.size())
                            break;

                        music1SectionPointer = readMusicWord(music1SectionPointer + 2);

                        if (music1SectionPointer + 1 >= musicRom.size())
                            break;

                        section = readMusicWord(music1SectionPointer);
                    }

                    if (section == 0 || section >= musicRom.size())
                        break;

                    music1Cursor = section;
                    continue;
                }

                if (command == 0x9D)
                {
                    if (music1Cursor + 3 > musicRom.size())
                        break;

                    music1Envelope = musicRom[music1Cursor];
                    music1DutyLength = musicRom[music1Cursor + 2];

                    music1Cursor += 3;
                    continue;
                }

                if ((command & 0xF0) == 0xA0)
                {
                    int tempoIndex = command & 0x0F;

                    if (musicTempoTable + tempoIndex >= musicRom.size())
                        break;

                    music1TempoFrames = musicRom[musicTempoTable + tempoIndex];

                    continue;
                }

                double durationMs = music1TempoFrames * 1000.0 / 59.73;

                if (durationMs < 1.0) durationMs = 1.0;

                if (command == 0x01)
                {
                    music1NoteOn = false;

                    if (rotationSfxUntil == 0)
                        pulse1.enabled = false;

                    nextMusic1NoteAt += durationMs;
                    scheduled = true;
                    break;
                }

                if (command >= 2 && command <= 0x90 && (command % 2) == 0)
                {
                    size_t frequencyOffset = 0x6E04 + command - 2;

                    if (frequencyOffset + 1 >= musicRom.size())
                        break;

                    music1CurrentPeriod = readMusicWord(frequencyOffset);

                    music1NoteOn = true;

                    if (rotationSfxUntil == 0) playMusic1Note();

                    nextMusic1NoteAt += durationMs;
                    scheduled = true;
                    break;
                }

                SDL_Log("Unsupported Channel 1 music command: 0x%02X", command);

                break;
            }

            if (!scheduled)
            {
                SDL_Log("Channel 1 music decoder stopped.");

                musicPreviewPlaying = false;
                pulse1.enabled = false;
                pulse2.enabled = false;
            }
        }

        if (rotationSfxUntil != 0 && mode != MODE_PAUSED && SDL_GetTicks() >= rotationSfxUntil)
        {
            rotationSfxUntil = 0;

            playMusic1Note();
        }

        // ============================================================================
        // MUSIC CHANNEL 3
        // Decode and schedule the waveform accompaniment.
        // ============================================================================

        if (musicPreviewPlaying && mode != MODE_PAUSED && !tetrisWaveActive && music3Cursor != 0 && SDL_GetTicks() >= nextMusic3NoteAt)
        {
            bool scheduled = false;

            for (int attempt = 0; attempt < 64; attempt++)
            {
                if (music3Cursor >= musicRom.size())
                    break;

                uint8_t command = musicRom[music3Cursor++];

                if (command == 0x00)
                {
                    music3SectionPointer += 2;

                    if (music3SectionPointer + 1 >= musicRom.size())
                        break;

                    uint16_t nextSection = readMusicWord(music3SectionPointer);

                    if (nextSection == 0xFFFF)
                    {
                        if (music3SectionPointer + 3 >= musicRom.size())
                            break;

                        music3SectionPointer = readMusicWord(music3SectionPointer + 2);

                        if (music3SectionPointer + 1 >= musicRom.size())
                            break;

                        nextSection = readMusicWord(music3SectionPointer);
                    }
                    if (nextSection == 0 || nextSection >= musicRom.size())
                        break;

                    music3Cursor = nextSection;
                    continue;
                }
                if (command == 0x9D)
                {
                    if (music3Cursor + 3 > musicRom.size())
                        break;

                    uint16_t waveAddress = readMusicWord(music3Cursor);
                    uint8_t outputLevel = musicRom[music3Cursor + 2];

                    if (static_cast<size_t>(waveAddress) + 16 > musicRom.size())
                        break;

                    wave.writeNR30(0x00);

                    std::memcpy(wave.waveRAM, musicRom.data() + waveAddress, 16);

                    wave.writeNR31(0x00);
                    wave.writeNR32(outputLevel);

                    wave.writeNR30(0x80);

                    music3Cursor += 3;
                    continue;
                }

                if ((command & 0xF0) == 0xA0)
                {
                    size_t tempoAddress =
                    musicTempoTable + (command & 0x0F);

                    if (tempoAddress >= musicRom.size())
                        break;

                    music3TempoFrames = musicRom[tempoAddress];
                    continue;
                }

                double durationMs = music3TempoFrames * 1000.0 / 59.73;

                if (durationMs < 1.0) durationMs = 1.0;

                if (command == 0x01)
                {
                    wave.enabled = false;

                    nextMusic3NoteAt += durationMs;
                    scheduled = true;
                    break;
                }

                if (command >= 2 && command <= 0x90 && (command % 2) == 0)
                {
                    size_t frequencyAddress = 0x6E04 + command - 2;

                    if (frequencyAddress + 1 >= musicRom.size())
                        break;

                    uint16_t period = readMusicWord(frequencyAddress);

                    wave.writeNR33(static_cast<uint8_t>(period & 0xFF));

                    wave.writeNR34(static_cast<uint8_t>(0x80 | ((period >> 8) & 0x07)));

                    nextMusic3NoteAt += durationMs;
                    scheduled = true;
                    break;
                }

                SDL_Log("Unsupported Channel 3 command: 0x%02X", command);

                break;
            }

            if (!scheduled)
            {
                SDL_Log("Channel 3 music decoder stopped.");

                musicPreviewPlaying = false;

                pulse1.enabled = false;
                pulse2.enabled = false;
                wave.enabled = false;
            }
        }

        // ============================================================================
        // MUSIC CHANNEL 4
        // Decode and schedule noise-based percussion.
        // ============================================================================

        if (musicPreviewPlaying && mode != MODE_PAUSED && !tetrisWaveActive && music4Cursor != 0 && SDL_GetTicks() >= nextMusic4NoteAt)
        {
            bool scheduled = false;

            for (int attempt = 0; attempt < 32; attempt++)
            {
                if (music4Cursor >= musicRom.size())
                    break;

                uint8_t command = musicRom[music4Cursor++];

                if (command == 0x00)
                {
                    music4SectionPointer += 2;

                    if (music4SectionPointer + 1 >= musicRom.size())
                        break;

                    uint16_t section = readMusicWord(music4SectionPointer);

                    if (section == 0xFFFF)
                    {
                        if (music4SectionPointer + 3 >= musicRom.size())
                            break;

                        music4SectionPointer = readMusicWord(music4SectionPointer + 2);

                        if (music4SectionPointer + 1 >= musicRom.size())
                            break;

                        section = readMusicWord(music4SectionPointer);
                    }

                    if (section == 0 || section >= musicRom.size())
                        break;

                    music4Cursor = section;
                    continue;
                }

                if ((command & 0xF0) == 0xA0)
                {
                    size_t tempoAddress = musicTempoTable + (command & 0x0F);

                    if (tempoAddress >= musicRom.size())
                        break;

                    music4TempoFrames = musicRom[tempoAddress];
                    continue;
                }

                if (command >= 1 && command <= 16 && ((command - 1) % 5) == 0)
                {
                    size_t instrumentAddress = 0x6E95 + command - 1;

                    if (instrumentAddress + 4 >= musicRom.size())
                        break;

                    uint8_t nr42 = musicRom[instrumentAddress];
                    uint8_t nr41 = musicRom[instrumentAddress + 2];
                    uint8_t nr43 = musicRom[instrumentAddress + 3];
                    uint8_t nr44 = musicRom[instrumentAddress + 4];

                    if (SDL_GetTicks() >= noiseSfxUntil)
                    {
                        noise.writeNR41(nr41);
                        noise.writeNR42(nr42);
                        noise.writeNR43(nr43);
                        noise.writeNR44(static_cast<uint8_t>(nr44 | 0x80));
                    }

                    double durationMs = music4TempoFrames * 1000.0 / 59.73;

                    if (durationMs < 1.0) durationMs = 1.0;
                    nextMusic4NoteAt += durationMs;

                    scheduled = true;
                    break;
                }

                SDL_Log("Unsupported Channel 4 command: 0x%02X", command);
                break;
            }

            if (!scheduled)
            {
                SDL_Log("Channel 4 music decoder stopped.");
                musicPreviewPlaying = false;

                pulse1.enabled = false;
                pulse2.enabled = false;
                wave.enabled = false;

                if (SDL_GetTicks() >= noiseSfxUntil) noise.enabled = false;
            }
        }

        if (!musicPreviewPlaying && currentRomSong != 0)
        {
            stopRomSong();
        }

        // ============================================================================
        // REAL-TIME AUDIO MIXING
        // Fill the SDL audio queue by sampling the four active channels.
        // ============================================================================

        if (audioDevice != 0)
        {
            while (SDL_GetQueuedAudioSize(audioDevice) < AUDIO_TARGET_BYTES)
            {
                // determine which of the four channels are being used to create the sound effects
                const Uint32 audioNow = SDL_GetTicks();

                const bool pulse1Effect = audioNow < rotationSfxUntil ||
                    (screen == Screen::COPYRIGHT && startupChimeStep < 3) ||
                    (screen == Screen::IN_GAME && lineSfxType != 0);

                // fix: muted music playing when clearing lines
                const bool pulse2Effect = tetrisWaveActive ||
                    (screen == Screen::IN_GAME && mode == MODE_PAUSED);

                const bool waveEffect = tetrisWaveActive ||
                    (screen == Screen::IN_GAME && mode == MODE_GAME_OVER_FILL);

                const bool noiseEffect = tetrisWaveActive || audioNow < noiseSfxUntil ||
                    (screen == Screen::IN_GAME && mode == MODE_GAME_OVER_FILL);

                // this converts 0 to 100 percents to 0.0 - 1.0
                const float musicGain = musicVolume / 100.0f;
                const float effectGain = sfxVolume / 100.0f;
                const float masterGain = masterVolume / 100.0f;

                for (int i = 0; i < AUDIO_CHUNK_SAMPLES; i++)
                {
                    float channel1 = pulse1.nextSample(AUDIO_SAMPLE_RATE);

                    float channel2 = pulse2.nextSample(AUDIO_SAMPLE_RATE);

                    float channel3 = wave.nextSample(AUDIO_SAMPLE_RATE);

                    float channel4 = noise.nextSample(AUDIO_SAMPLE_RATE);

                    // apply music and SFX levels to each channel then apply master volume to the result
                    audioBuffer[i] = masterGain * (channel1 * (pulse1Effect ? effectGain : musicGain) +
                        channel2 * (pulse2Effect ? effectGain : musicGain) +
                        channel3 * (waveEffect ? effectGain : musicGain) +
                        channel4 * (noiseEffect ? effectGain : musicGain));
                }
                if (SDL_QueueAudio(audioDevice, audioBuffer.data(), static_cast<Uint32>(audioBuffer.size() * sizeof(float))) != 0)
                {
                    SDL_Log("Audio queue error: %s", SDL_GetError());
                    break;
                }
            }
        }

        // ============================================================================
        // RENDER: GAME BOY FRAME
        // Draw each menu or gameplay view into the original-resolution texture.
        // ============================================================================

        if (SDL_SetRenderTarget(renderer, gameFrame) != 0)
        {
            SDL_Log("Render target error: %s", SDL_GetError());
            running = false;
            break;
        }

        SDL_SetRenderDrawColor(renderer, PALETTE[0].r, PALETTE[0].g, PALETTE[0].b, 255);
        SDL_RenderClear(renderer);

        if (romLoaded && screen == Screen::COPYRIGHT)
        {
            drawLayout(renderer, titleTiles, copyrightLayout);
        }
        else if (romLoaded && screen == Screen::TITLE)
        {
            drawLayout(renderer, titleTiles, titleLayout);

            if (numPlayers == 0)
            {
                drawSpriteTile(renderer, titleTiles, 0x58, 8, 112);
            }
            else
            {
                drawSpriteTile(renderer, titleTiles, 0x58, 88, 112);
            }
        }
        else if (romLoaded && screen == Screen::GAME_MUSIC)
        {
            // Draw expanded menu using assets from the rom, SDL could do it but it wont look great 
            drawLayout(renderer, tiles, expandedGameMusicLayout);

            const bool flashOn = (SDL_GetTicks() / 268) % 2 == 0;

            const char* extrasLabel = "EXTRAS";

            const int dimText[4] = { 0, 1, 1, 1 };
            const int brightText[4] = { 0, 3, 3, 3 };

            bool extrasHighlighted = !musicMenuFocus && selectedGameType == 3 && flashOn;

            for (int i = 0; i < 6; ++i)
            {
                int tile = 0x0A + (extrasLabel[i] - 'A');

                drawTile(renderer, tiles.data() + tile * 16, 88 + i * 8, 56, extrasHighlighted ? brightText : dimText);
            }

            // OG selection graphics for A, B and C
            if (!musicMenuFocus && flashOn && selectedGameType < 3)
            {
                int selectedX = (selectedGameType % 2 == 0) ? 24 : 88;

                int selectedY = (selectedGameType < 2) ? 40 : 56;

                drawOriginalMenuSelection(renderer, musicRom, tiles, selectedGameType, selectedX, selectedY);
            }

            // Music Type has moved down by one tile
            int musicX = (selectedMusic % 2 == 0) ? 24 : 88;

            int musicY = (selectedMusic < 2) ? 104 : 120;

            if (!musicMenuFocus || flashOn)
            {
                drawOriginalMenuSelection(renderer, musicRom, tiles, selectedMusic, musicX, musicY);
            }
        }
        else if (romLoaded && screen == Screen::SETTINGS)
        {
            if (settingsPage == 0)
            {
            // Background and panels from the OG ROM
            drawLayout(renderer, tiles, extrasLayout);

            drawGBText(renderer, tiles, "EXTRAS", 7, 2);
            drawGBText(renderer, tiles, "P1", 16, 2);

            // Palette panel
            drawGBText(renderer, tiles, "COLOR SELECTION", 3, 5);
            drawGBText(renderer, tiles, PALETTE_NAMES[paletteIndex], 3, 6);

            for (int i = 0; i < 4; ++i)
            {
                SDL_SetRenderDrawColor(renderer,
                    PALETTE[i].r, PALETTE[i].g, PALETTE[i].b, 255);

                SDL_Rect swatch = { 24 + i * 30, 57, 26, 6 };
                SDL_RenderFillRect(renderer, &swatch);
            }

            // fullscreen should onto one row
            drawGBText(renderer, tiles, "FULLSCREEN", 3, 10);

            bool fullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;

            drawGBText(renderer, tiles, fullscreen ? "ON" : "OFF", 15, 10);

            // the three gameplay options
            drawGBText(renderer, tiles, "GHOST PIECE", 3, 13);
            drawGBText(renderer, tiles, ghostEnabled ? "ON" : "OFF", 15, 13);

            drawGBText(renderer, tiles, "HARD DROP", 3, 14);
            drawGBText(renderer, tiles, hardDropEnabled ? "ON" : "OFF", 15, 14);

            drawGBText(renderer, tiles, "HOLD PIECE", 3, 15);
            drawGBText(renderer, tiles, holdEnabled ? "ON" : "OFF", 15, 15);

            // le cursor should follow all 5 settings
            const int cursorRows[5] = { 5, 10, 13, 14, 15 };
            int cursorRow = cursorRows[settingsSelection];


            SDL_SetRenderDrawColor(renderer, PALETTE[3].r, PALETTE[3].g, PALETTE[3].b, 255);

            SDL_Rect cursor = { 17, cursorRow * 8 + 2, 4, 5 };
            SDL_RenderFillRect(renderer, &cursor);

            }
            else // extras Page 2
            {
                // OG rom background and borders
                drawLayout(renderer, tiles, extrasAudioLayout);

                drawGBText(renderer, tiles, "EXTRAS", 7, 2);
                drawGBText(renderer, tiles, "P2", 16, 2);

                // the big three 
                const char* names[3] =
                {
                    "MASTER VOL",
                    "MUSIC VOL",
                    "SFX VOL"
                };

                const int volumes[3] =
                {
                    masterVolume,
                    musicVolume,
                    sfxVolume
                };

                const int rowStarts[3] = { 5, 9, 13 };

                for (int opt = 0; opt < 3; ++opt)
                {
                    int row = rowStarts[opt];

                    // name and Percent
                    drawGBText(renderer, tiles,
                        names[opt], 3, row);

                    drawNumber(renderer, tiles,
                        volumes[opt], 3, 144, row * 8);

                    // use blocks to form the volume bar (should be exactly 10)
                    for (int j = 0; j < 10; ++j)
                    {
                        const SDL_Color shade =
                            (j < (volumes[opt] + 9) / 10)
                            ? PALETTE[3]
                            : PALETTE[1];

                        SDL_SetRenderDrawColor(renderer,
                            shade.r, shade.g, shade.b, 255);

                        SDL_Rect tick =
                        {
                            24 + j * 11,
                            (row + 1) * 8 + 2,
                            9,
                            4
                        };

                        SDL_RenderFillRect(renderer, &tick);
                    }
                }

                // TOTAL PLAYTIME (first on to 1000 hours gets a pack of Chocolate Digestives)
                uint64_t totalSeconds = savedPlaySeconds + sessionPlayMs / 1000;

                std::string totalText =
                    "TOTAL " + std::to_string(totalSeconds / 3600) + 
                    "H " + std::to_string((totalSeconds / 60) % 60) + "M";

                if (totalText.size() > 16) totalText.resize(16);

                drawGBText(renderer, tiles, totalText, 2, 16);

                // selection cursor 
                int cursorRow = rowStarts[settingsSelection];

                SDL_SetRenderDrawColor(renderer, PALETTE[3].r, PALETTE[3].g, PALETTE[3].b, 255);

                SDL_Rect cursor =
                {
                    17, cursorRow * 8 + 2, 4, 5
                };

                SDL_RenderFillRect(renderer, &cursor);
            }
        }
        else if (romLoaded && (screen == Screen::A_LEVEL || screen == Screen::SCOREBOARD))
        {
            drawLayout(renderer, tiles, aSelectLayout);

            int shownLevel = (screen == Screen::SCOREBOARD) ? playedLevel : aLevel;

            for (int i = 0; i < 3; i++)
            {
                const HighScore& entry = highScores[shownLevel][i];

                if (entry.score <= 0) continue;

                drawScoreName(renderer, tiles, entry.name, 4, 13 + i);

                drawNumber(renderer, tiles, entry.score, 0, 144, (13 + i) * 8);
            }

            int x = 40 + (aLevel % 5) * 16;
            int y = 48 + (aLevel / 5) * 16;

            bool cursorVisible = (SDL_GetTicks() / 268) % 2 == 0;

            if (screen == Screen::A_LEVEL && cursorVisible)
            {
                drawSpriteTile(renderer, tiles, aLevel, x, y);
            }
        }
        else if (romLoaded && screen == Screen::IN_GAME)
        {
            drawLayout(renderer, tiles, layout);

            drawNumber(renderer, tiles, score, 0, 152, 24);
            drawNumber(renderer, tiles, level, 0, 152, 56);
            drawNumber(renderer, tiles, lines, 0, 152, 80);

            if (mode == MODE_PAUSED)
            {
                drawTextBox(renderer, tiles, pauseText, 8, 10, 3, 3);
            }
            else if (mode == MODE_GAME_OVER_TEXT)
            {
                Uint32 elapsed = SDL_GetTicks() - gameOverRevealStart;
                int revealedRows = 1 + static_cast<int>(elapsed * 59.7275 / 1000.0);

                if (revealedRows > FIELD_H) revealedRows = FIELD_H;

                const int SOLID_TILE = 0x87;
                const int EMPTY_TILE = 0x2F;

                for (int r = 0; r < FIELD_H; r++)
                {
                    bool revealed = r >= FIELD_H - revealedRows;

                    for (int c = 0; c < FIELD_W; c++)
                    {
                        int tile = SOLID_TILE;

                        if (revealed)
                        {
                            tile = EMPTY_TILE;

                            if (r >= 2 && r < 9 && c >= 1 && c < 9 && gameOverText.size() >= 56)
                            {
                                tile = gameOverText[(r - 2) * 8 + (c - 1)];
                            }
                            else if (r >= 12 && r < 18 && c >= 1 && c < 9 && tryAgainText.size() >= 48)
                            {
                                tile = tryAgainText[(r - 12) * 8 + (c - 1)];
                            }
                        }

                        drawTile(renderer, tiles.data() + tile * 16, FIELD_X + c * 8, FIELD_Y + r * 8);
                    }
                }
            }
            else
            {
                drawField(renderer, tiles);

                if (mode == MODE_GAME_OVER_FILL)
                {

                    Uint32 elapsed = SDL_GetTicks() - modeStart;
                    for (int r = 0; r < FIELD_H; r++)
                        if (elapsed >= (Uint32)(FIELD_H - 1 - r) * 17)
                            for (int c = 0; c < FIELD_W; c++)
                                drawTile(renderer, tiles.data() + 0x87 * 16,
                        FIELD_X + c * 8, FIELD_Y + r * 8);
                }
                else
                {
                    if (holdEnabled && heldFrame)
                    {
                        drawPreviewLabel(renderer, tiles, "NEXT", 120, 104);
                    }

                    // always use the original tetromino preview
                    // tetromino is a fun name :D its got a nice pronounciatiom
                    if (!nextHidden)
                    {
                        drawPiece(renderer, tiles, Piece{ nextType, 0, 13, 13 });
                    }

                    if (clearing)
                    {
                        const int FLASH_TILE = 0x8C;
                        const int EMPTY_TILE = 0x2F;

                        Uint32 elapsed = SDL_GetTicks() - clearStart;

                        if (elapsed < SHIFT_START_MS)
                        {

                            int phase = static_cast<int>(elapsed / FLASH_MS);

                            int tile = -1;

                            if (phase >= 6) tile = EMPTY_TILE;

                            else if (phase % 2 == 0) tile = FLASH_TILE;

                            if (tile >= 0)
                            {
                                for (int r = 0; r < FIELD_H; r++)
                                {
                                    if (!rowIsFull(r))
                                        continue;

                                    for (int c = 0; c < FIELD_W; c++)
                                    {
                                        drawTile(renderer, tiles.data() + tile * 16, FIELD_X + c * 8, FIELD_Y + r * 8);
                                    }
                                }
                            }
                        }
                        else
                        {

                            int rowsToReveal = 1 + static_cast<int>((elapsed - SHIFT_START_MS) / ROW_REVEAL_MS);

                            if (rowsToReveal > FIELD_H) rowsToReveal = FIELD_H;

                            for (int r = FIELD_H - rowsToReveal;
                                r < FIELD_H;
                                r++)
                            {
                                for (int c = 0; c < FIELD_W; c++)
                                {
                                    int tile = shiftedField[r][c];

                                    if (tile < 0) tile = EMPTY_TILE;

                                    drawTile(renderer, tiles.data() + tile * 16, FIELD_X + c * 8, FIELD_Y + r * 8);
                                }
                            }
                        }
                    }
                    else if (entryPhase == 0)
                    {
                        if (ghostEnabled && mode == MODE_PLAYING)
                        {
                            // make a copy of the current piece
                            Piece ghost = piece;

                            // move the copy downward
                            while (!collides(ghost))
                            {
                                Piece below = ghost;
                                below.y++;

                                if (collides(below))
                                    break;

                                ghost = below;
                            }

                            // draw the ghost only if it's below the real piece
                            if (ghost.y != piece.y && !collides(ghost))
                            {
                                drawGhostPiece(renderer, ghost);
                            }
                        }

                        // draw the actual falling piece on top
                        drawPiece(renderer, tiles, piece);
                    }

                }
            }
        }

        // ============================================================================
        // RENDER: WINDOW PRESENTATION
        // Scale the game frame, extend the game panel and present the output.
        // ============================================================================

        // draw the timer in its own render target
        const bool showGameTimer = romLoaded && screen == Screen::IN_GAME && gameTimerFrame;

        if (showGameTimer)
        {
            if (SDL_SetRenderTarget(renderer, gameTimerFrame) == 0)
            {
                drawGameTimer(renderer, tiles, layout, gamePlayMs);
            }
        }

        const bool showHeldPanel = romLoaded && screen == Screen::IN_GAME && holdEnabled && heldFrame;

        if (showHeldPanel)
        {
            if (SDL_SetRenderTarget(renderer, heldFrame) == 0)
            {
                SDL_SetRenderDrawColor(renderer, PALETTE[3].r, PALETTE[3].g, PALETTE[3].b, 255);

                SDL_RenderClear(renderer);

                // Hide HELD during pause and Game Over.
                const bool hideHeldContents = mode == MODE_PAUSED || mode == MODE_GAME_OVER_FILL || mode == MODE_GAME_OVER_TEXT;

                drawHeldPreview(renderer, tiles, layout, hideHeldContents ? -1 : heldPieceType, !hideHeldContents);
            }
        }

        SDL_SetRenderTarget(renderer, nullptr);

        // the Copyright screen should always use the players selected palette
        if (screen == Screen::COPYRIGHT)
        {
            SDL_SetRenderDrawColor(renderer, PALETTE[0].r, PALETTE[0].g, PALETTE[0].b, 255);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, PALETTE[3].r, PALETTE[3].g, PALETTE[3].b, 255);
        }
        SDL_RenderClear(renderer);

        SDL_Rect gameDst = getGameDstRect(renderer);

        // make room for the OG screen and the new held panel
        if (romLoaded && screen == Screen::IN_GAME && (showGameTimer || showHeldPanel))
        {
            int windowW, windowH;
            SDL_GetRendererOutputSize(renderer, &windowW, &windowH);

            float scaleX = windowW / 220.0f;
            float scaleY = windowH / 144.0f;
            float scale = (scaleX < scaleY) ? scaleX : scaleY;

            int totalW = static_cast<int>(std::floor(220.0f * scale));
            int frameW = static_cast<int>(std::floor(160.0f * scale));
            int frameH = static_cast<int>(std::floor(144.0f * scale));

            gameDst = {(windowW - totalW) / 2, (windowH - frameH) / 2, frameW, frameH};
        }

        SDL_Rect fullSrc = { 0, 0, 160, 144 };

        SDL_RenderCopy(renderer, gameFrame, &fullSrc, &gameDst);

        if (romLoaded && screen == Screen::IN_GAME)
        {
            int windowW, windowH;
            SDL_GetRendererOutputSize(renderer, &windowW, &windowH);

            int extensionX = gameDst.x + gameDst.w;
            int extensionW = windowW - extensionX;

            if (extensionW > 0)
            {
                SDL_Rect sliceSrc = { 159, 0, 1, 144 };

                SDL_Rect sliceDst = {extensionX, gameDst.y, extensionW, gameDst.h};

                if (showGameTimer || showHeldPanel)
                {
                    sliceSrc.h = 96;

                    sliceDst.h = static_cast<int>(gameDst.h * (96.0f / 144.0f));
                }

                SDL_RenderCopy(renderer, gameFrame, &sliceSrc, &sliceDst);
            }
        }

        // display time next to level
        if (showGameTimer)
        {
            float scale = gameDst.w / 160.0f;

            SDL_Rect timerDst =
            {
                // make the new box appear four GB pixels to the right 
                gameDst.x + static_cast<int>(164.0f * scale),

                // align next to the OG level box 
                gameDst.y + static_cast<int>(40.0f * scale),

                // use the same dimentions from the OG level box
                static_cast<int>(56.0f * scale),
                static_cast<int>(28.0f * scale)
            };

            SDL_RenderCopy(renderer, gameTimerFrame, nullptr, &timerDst);
        }

        // held must be directly to the right of the next panel
        if (showHeldPanel)
        {
            float scale = gameDst.w / 160.0f;

            SDL_Rect heldDst =
            {
                gameDst.x + static_cast<int>(164.0f * scale),
                gameDst.y + static_cast<int>(96.0f * scale),
                static_cast<int>(56.0f * scale),
                static_cast<int>(48.0f * scale)
            };

            SDL_RenderCopy(renderer, heldFrame, nullptr, &heldDst);
        }

        SDL_RenderPresent(renderer);
    }

    // ============================================================================
    // SHUTDOWN
    // Release Discord, SDL graphics, audio, input and the window.
    // ============================================================================

    savePreferences();
    discordClient.reset();
    SDL_DestroyTexture(gameTimerFrame);
    SDL_DestroyTexture(heldFrame);
    SDL_DestroyTexture(gameFrame);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    if (audioDevice != 0)
    {
        SDL_CloseAudioDevice(audioDevice);
    }
    if (controller) SDL_GameControllerClose(controller);
    SDL_Quit();
    return 0;
}
