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
#include <cmath>
#include <memory>
#include <ctime>
#include <chrono>
#include "discordpp.h"

// ============================================================================
// PROJECT DEFINITIONS
// Screen states, score entries, piece data, palette and cartridge constants.
// ============================================================================

const size_t ROM_SIZE = 32768;

enum class Screen
{
    COPYRIGHT,
    TITLE,
    GAME_MUSIC,
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

const SDL_Color PALETTE[4] = {
    {155, 188,  15, 255},
    {139, 172,  15, 255},
    { 48,  98,  48, 255},
    { 15,  56,  15, 255},
};

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

            file << entry.score << " " << (entry.name.empty() ? "------" : entry.name) << "\n";
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

            if (value < 0 || value > 999999 || name.size() > 6)
                return;

            if (name == "------") name = "";

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
    std::vector<uint8_t> aSelectLayout;
    std::vector<uint8_t> bSelectLayout;
    std::vector<uint8_t> pauseText, gameOverText, tryAgainText;

    bool romLoaded = false;

    Screen screen = Screen::COPYRIGHT;
    Uint32 screenStart = SDL_GetTicks();

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

        hiddenType = 0;
        nextType = 0;

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

            highScores[playedLevel][pendingRank] = { "AAAAAA", score };

            namePosition = 0;
        }

        if (pendingRank >= 0)
        {
            startRomSong(1);
        }
        else
        {
            resumeChosenMusic();
        }
        screen = Screen::SCOREBOARD;
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

                    SDL_Log("Controller disconnected");
                }
            }
            else if (event.type == SDL_DROPFILE)
            {
                tryLoadRom(event.drop.file);
                SDL_free(event.drop.file);
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

                    case SDL_CONTROLLER_BUTTON_BACK:
                        key = SDLK_F11;
                        break;

                    default:
                        continue;
                    }
                }

                if (key == SDLK_F6)
                {
                    if (musicPreviewPlaying && currentRomSong == 5)
                    {
                        stopRomSong();
                    }
                    else
                    {
                        startRomSong(5);
                    }

                    continue;
                }

                if (key == SDLK_F11)
                {

                    bool fullscreen =
                    (SDL_GetWindowFlags(window) &
                        SDL_WINDOW_FULLSCREEN) != 0;

                    if (SDL_SetWindowFullscreen(window, fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP) != 0)
                    {
                        SDL_Log("Fullscreen error: %s", SDL_GetError());
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

                            if (key == SDLK_LEFT && selectedGameType != 0)
                            {
                                selectedGameType = 0;
                                selectionChanged = true;
                            }
                            else if (key == SDLK_RIGHT && selectedGameType != 1)
                            {
                                selectedGameType = 1;
                                selectionChanged = true;
                            }
                            else if (key == SDLK_DOWN)
                            {
                                musicMenuFocus = true;
                                focusChanged = true;
                            }
                        }
                        else
                        {
                            int previousMusic = selectedMusic;

                            if (key == SDLK_LEFT && selectedMusic % 2 == 1)
                                selectedMusic--;

                            else if (key == SDLK_RIGHT && selectedMusic % 2 == 0)
                                selectedMusic++;

                            else if (key == SDLK_DOWN && selectedMusic < 2)
                                selectedMusic += 2;

                            else if (key == SDLK_UP)
                            {
                                if (selectedMusic >= 2)
                                    selectedMusic -= 2;
                                else
                                {
                                    musicMenuFocus = false;
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
                        if (selectionChanged || focusChanged)
                        {
                            playMenuSound(false);
                        }
                        if (key == SDLK_RETURN)
                        {
                            if (selectedGameType == 0)
                            {
                                playMenuSound(true);

                                screen = Screen::A_LEVEL;
                            }
                            else
                            {
                                SDL_Log("B-Type is not implemented yet.");
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

                            if (key == SDLK_UP)
                            {
                                if (name[namePosition] == 'Z')
                                    name[namePosition] = 'A';
                                else
                                    name[namePosition]++;
                            }
                            else if (key == SDLK_DOWN)
                            {
                                if (name[namePosition] == 'A')
                                    name[namePosition] = 'Z';
                                else
                                    name[namePosition]--;
                            }
                            else if (key == SDLK_x || key == SDLK_SPACE)
                            {
                                if (namePosition < 5)
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
                for (int i = 0; i < AUDIO_CHUNK_SAMPLES; i++)
                {
                    float channel1 = pulse1.nextSample(AUDIO_SAMPLE_RATE);

                    float channel2 = pulse2.nextSample(AUDIO_SAMPLE_RATE);

                    float channel3 = wave.nextSample(AUDIO_SAMPLE_RATE);

                    float channel4 = noise.nextSample(AUDIO_SAMPLE_RATE);

                    audioBuffer[i] = channel1 + channel2 + channel3 + channel4;
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

        SDL_SetRenderDrawColor(renderer, 155, 188, 15, 255);
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
            drawLayout(renderer, tiles, gameMusicLayout);
            bool flashOn = (SDL_GetTicks() / 268) % 2 == 0;

            int gameX = (selectedGameType == 0) ? 24 : 88;
            int gameY = 40;

            if (musicMenuFocus || flashOn)
            {
                drawOriginalMenuSelection(renderer, musicRom, tiles, selectedGameType, gameX, gameY);
            }

            int musicX = (selectedMusic % 2 == 0) ? 24 : 88;
            int musicY = (selectedMusic < 2) ? 96 : 112;

            if (!musicMenuFocus || flashOn)
            {
                drawOriginalMenuSelection(renderer, musicRom, tiles, selectedMusic, musicX, musicY);
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

                drawGBText(renderer, tiles, entry.name, 4, 13 + i);

                drawNumber(renderer, tiles, entry.score, 0, 144, (13 + i) * 8);
            }

            int x = 40 + (aLevel % 5) * 16;
            int y = 48 + (aLevel / 5) * 16;

            bool cursorVisible = (SDL_GetTicks() / 268) % 2 == 0;

            if (cursorVisible)
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
                    if (!nextHidden)
                        drawPiece(renderer, tiles, Piece{ nextType, 0, 13, 13 });

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
                        drawPiece(renderer, tiles, piece);
                    }
                }
            }
        }

        // ============================================================================
        // RENDER: WINDOW PRESENTATION
        // Scale the game frame, extend the game panel and present the output.
        // ============================================================================

        SDL_SetRenderTarget(renderer, nullptr);

        if (screen == Screen::COPYRIGHT)
        {
            SDL_SetRenderDrawColor(renderer, 155, 188, 15, 255);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 15, 56, 15, 255);
        }

        SDL_RenderClear(renderer);

        SDL_Rect gameDst = getGameDstRect(renderer);

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

                SDL_Rect sliceDst = {
                    extensionX,
                    gameDst.y,
                    extensionW,
                    gameDst.h
                };

                SDL_RenderCopy(renderer, gameFrame, &sliceSrc, &sliceDst);
            }
        }

        SDL_RenderPresent(renderer);
    }

    // ============================================================================
    // SHUTDOWN
    // Release Discord, SDL graphics, audio, input and the window.
    // ============================================================================

    discordClient.reset();
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
