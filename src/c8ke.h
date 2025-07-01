#pragma once

// custom definitions
using byte = unsigned char; // 8 bits, 1 byte
using word = unsigned short; // 16 bits, 2 bytes

// emulator values
const unsigned short CLK = 500; // 500 Hz, 500 cycles/sec
const double TIME_PER_CYCLE = 1000000000.0 / CLK;
const unsigned char FPS = 60; // 60 FPS, 60 frames/sec
const double TIME_PER_REFRESH = 1000000000.0 / FPS;
const unsigned short MAX_MEM = 4096; // 4KB memory, 4096 bites
const unsigned short START_ADDRESS = 0x200; // memory start address
std::string romPath = "";

// display values
const unsigned char WIDTH = 64; // original interpreter screen width
const unsigned char HEIGHT = 32; // original interpreter screen height
byte screen[HEIGHT][WIDTH] = { 0 }; // original interpreter screen
float SCALE = 11; // scale emulator screen for modern monitors
int WINDOW_WIDTH = 1000; // actual window width
int WINDOW_HEIGHT = 800; // actual window heights

// audio values
const int DEFAULT_BEEP_AMOUNT = 100; // beep parameter 1
const int DEFAULT_BEEP_PHASE = 2200; // beep parameter 2
int current_sine_sample = 0; // something for the beep
struct CustomAudio {
	int beepAmount = DEFAULT_BEEP_AMOUNT; 
	int beepPhase = DEFAULT_BEEP_PHASE; 
};
CustomAudio customAudio;

// state machine for the emulator
enum State {
	INIT,
	RUNNING,
	QUIT,
	PAUSED,
	RELOAD,
	DELAYED,
	HALT,
	DELAY_HALT,
	RESET,
};
static State c8keState = INIT;

// input keys
std::unordered_map<SDL_Keycode, byte> keymap = { 
		{SDLK_1, 0x1}, {SDLK_2, 0x2}, {SDLK_3, 0x3}, {SDLK_4, 0xC},
		{SDLK_Q, 0x4}, {SDLK_W, 0x5}, {SDLK_E, 0x6}, {SDLK_R, 0xD},
		{SDLK_A, 0x7}, {SDLK_S, 0x8}, {SDLK_D, 0x9}, {SDLK_F, 0xE},
		{SDLK_Z, 0xA}, {SDLK_X, 0x0}, {SDLK_C, 0xB}, {SDLK_V, 0xF},
};
bool input[16] = { 0 }; // has pressed keys
byte tempReg = 0; // needed for one input instruction
byte chip8Keys[4][4] = { // for drawing debug controls
	{0x1, 0x2, 0x3, 0xC},
	{0x4, 0x5, 0x6, 0xD},
	{0x7, 0x8, 0x9, 0xE},
	{0xA, 0x0, 0xB, 0xF},
};

// default chip8 sprites
const unsigned char SPRITE_ADDRESS = 0x50; // beginning sprite address in memory
const unsigned char TOTAL_SPRITE_SIZE = 80; // total number of bytes the sprites take up
byte sprites[TOTAL_SPRITE_SIZE] = { // sprites to store in memory
			0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
			0x20, 0x60, 0x20, 0x20, 0x70, // 1
			0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
			0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
			0x90, 0x90, 0xF0, 0x10, 0x10, // 4
			0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
			0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
			0xF0, 0x10, 0x20, 0x40, 0x40, // 7
			0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
			0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
			0xF0, 0x90, 0xF0, 0x90, 0x90, // A
			0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
			0xF0, 0x80, 0x80, 0x80, 0xF0, // C
			0xE0, 0x90, 0x90, 0x90, 0xE0, // D
			0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
			0xF0, 0x80, 0xF0, 0x80, 0x80, // D
};


// SDL
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* texture = nullptr;
SDL_Surface* icon = nullptr;
SDL_AudioStream* stream = nullptr;
SDL_AudioSpec spec = { SDL_AUDIO_F32, 1, 8000 }; // format, channels, frequency
SDL_Event e;


// ImGui
ImFont* myFont;

const ImGuiColorEditFlags COLOR_EDIT_FLAGS_COLOR_BUTTON = ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_AlphaPreviewHalf;
const ImGuiColorEditFlags COLOR_EDIT_FLAGS_PICKER = ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview;

const ImGuiWindowFlags nonscrollable = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavFocus;
const ImGuiWindowFlags scrollable = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar;
const ImGuiWindowFlags emulatorScreen = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

bool showFgPicker = false;
bool showBgPicker = false;
bool showDbgColor1Picker = false;
bool showDbgColor2Picker = false;
bool showDbgColor3Picker = false;
bool showDbgBgPicker = false;
bool showDbgHeaderFgPicker = false;
bool showDbgHeaderBgPicker = false;

const ImVec4 DEFAULT_EMULATOR_FG = ImVec4(255.0f / 255.0f, 161.0f / 255.0f, 105.0f / 255.0f, 1.0f); // pastel orange
const ImVec4 DEFAULT_EMULATOR_BG = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); // black
const ImVec4 DEFAULT_DEBUG_COLOR_1 = ImVec4(255.0f / 255.0f, 161.0f / 255.0f, 105.0f / 255.0f, 1.0f); // pastel orange
const ImVec4 DEFAULT_DEBUG_COLOR_2 = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // white
const ImVec4 DEFAULT_DEBUG_COLOR_3 = ImVec4(75.0f / 255.0f, 75.0f / 255.0f, 75.0f / 255.0f, 1.0f); // gray
const ImVec4 DEFAULT_DEBUG_BG = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); // black
const ImVec4 DEFAULT_DEBUG_HEADER_FG = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // white
const ImVec4 DEFAULT_DEBUG_HEADER_BG = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); // black

struct CustomColors {
	ImVec4 emuFg = DEFAULT_EMULATOR_FG;
	ImVec4 emuBg = DEFAULT_EMULATOR_BG;
	ImVec4 dbgColor1 = DEFAULT_DEBUG_COLOR_1;
	ImVec4 dbgColor2 = DEFAULT_DEBUG_COLOR_2;
	ImVec4 dbgColor3 = DEFAULT_DEBUG_COLOR_3;
	ImVec4 dbgBg = DEFAULT_DEBUG_BG;
	ImVec4 dbgHeaderFg = DEFAULT_DEBUG_HEADER_FG;
	ImVec4 dbgHeaderBg = DEFAULT_DEBUG_HEADER_BG;
};
CustomColors customColors;