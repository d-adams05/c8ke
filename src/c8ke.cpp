#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <random>
#include <ios>
#include <unordered_map>

#include <SDL3/SDL.h> // v3.2.16
#include <SDL3/SDL_main.h> // v3.2.16

using byte = unsigned char; // 8 bits, 1 bite
using word = unsigned short; // 16 bits, 2 bites

const unsigned short CLK = 500; // 500 Hz, 500 cycles/sec
const unsigned char FPS = 60; // 60 FPS, 60 frames/sec
const unsigned short MAX_MEM = 4096; // 4KB memory, 4096 bites
const unsigned short START_ADDRESS = 0x200; // memory start address

const unsigned char WIDTH = 64; // original interpreter screen width
const unsigned char HEIGHT = 32; // original interpreter screen height
byte screen[HEIGHT][WIDTH] = { 0 }; // original interpreter screen
const unsigned char SCALE = 10; // scale for modern monitors
constexpr int WINDOW_WIDTH = WIDTH * SCALE; // actual window width
constexpr int WINDOW_HEIGHT = HEIGHT * SCALE; // actual window height

int foreground = 0xFAC898FF; // foreground color
int background = 0x000000FF; // background color
byte fr = (foreground >> 24) & 0xFF; // foreground red
byte fg = (foreground >> 16) & 0xFF; // foreground green
byte fb = (foreground >> 8) & 0xFF; // foreground blue
byte fa = foreground & 0xFF; // foreground alpha
byte br = (background >> 24) & 0xFF; // background red
byte bg = (background >> 16) & 0xFF; //background green
byte bb = (background >> 8) & 0xFF; // background blue
byte ba = background & 0xFF; // background alpha

int beepAmount = 100; // beep parameter 1
int beepPhase = 2200; // beep parameter 2
int current_sine_sample = 0; // something for the beep

enum State { // state machine for the emulator
	INIT = -1,
	RUNNING = 0,
	QUIT = 1,
	PAUSED = 2,
};
State c8keState = INIT; // holds current state of the emulator

std::unordered_map<SDL_Keycode, byte> keymap = { // keyboard keys to interpreter keys
		{SDLK_1, 0x1}, {SDLK_2, 0x2}, {SDLK_3, 0x3}, {SDLK_4, 0xC},
		{SDLK_Q, 0x4}, {SDLK_W, 0x5}, {SDLK_E, 0x6}, {SDLK_R, 0xD},
		{SDLK_A, 0x7}, {SDLK_S, 0x8}, {SDLK_D, 0x9}, {SDLK_F, 0xE},
		{SDLK_Z, 0xA}, {SDLK_X, 0x0}, {SDLK_C, 0xB}, {SDLK_V, 0xF},
};
bool input[16] = { 0 }; // has pressed keys
byte tempReg = 0; // needed for one input instruction

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

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_AudioStream* stream = nullptr;
SDL_AudioSpec spec = { SDL_AUDIO_F32, 1, 8000 }; // format, channels, frequency
SDL_Event e;


/**********************************************************************************************/


struct c8ke {
	word pc{}; // 16-bit program counter
	byte sp{}; // 8-bit stack pointer

	word stack[16]{}; // 16 16-bit values
	byte regs[16]{}; // 16 8-bit registers
	byte mem[MAX_MEM]{}; // program memory

	word iReg{}; // 16-bit i register
	byte delayReg{}; // 8-bit delay timer register
	byte soundReg{}; // 8-bit sound timer register

	void reset() {
		// reset values
		pc = START_ADDRESS;
		sp = -1;
		iReg = 0;
		delayReg = 0;
		soundReg = 0;
		for (byte i = 0; i < 16; i++) {
			stack[i] = 0;
			regs[i] = 0;
		}

		// load sprites into memory
		for (int i = 0; i < TOTAL_SPRITE_SIZE; i++) {
			mem[SPRITE_ADDRESS + i] = sprites[i];
		}

		//loadRom("res\\test\\3-corax+.ch8");
		//loadRom("res\\test\\4-flags.ch8");
		//loadRom("res\\test\\5-quirks.ch8");
		//loadRom("res\\test\\6-keypad.ch8");
		//loadRom("res\\test\\7-beep.ch8");
		loadRom("res\\games\\Tetris [Fran Dachille, 1991].ch8");
	}

	void loadRom(std::string path) {
		std::ifstream rom(path, std::ios::binary); // open file in binary mode
		if (!rom.is_open()) {
			std::cerr << "Error Opening File" << std::endl;
			exit(1);
		}

		byte byte;
		while (rom.read(reinterpret_cast<char*>(&byte), sizeof(byte))) { // read each line until there are no more lines
			mem[pc++] = byte;
		}

		pc = START_ADDRESS;
		rom.close();
	}

	void cycle() {
		word instruction = (mem[pc] << 8) | mem[pc + 1];
		pc += 2;

		switch (instruction & 0xF000) { // checks the first nibble
		case 0x0000: { // 00E*
			switch (instruction & 0x000F) {
			case 0x0: // 00E0: clear the display
				for (int y = 0; y < HEIGHT; y++) {
					for (int x = 0; x < WIDTH; x++) {
						screen[y][x] = 0;
					}
				}
				break;
			case 0xE: // 00EE: return from a subroutine
				pc = stack[sp];
				sp--;
				break;
			}
		} break;

		case 0x1000: { // 1nnn: jump to location nnn
			pc = instruction & 0x0FFF;
		} break;

		case 0x2000: { // 2nnn: call subroutine at nnn
			sp++;
			stack[sp] = pc;
			pc = instruction & 0x0FFF;
		} break;

		case 0x3000: { // 3xkk: skip next instruction if Vx = kk
			if (regs[(instruction & 0x0F00) >> 8] == (instruction & 0x00FF)) pc += 2;
		} break;

		case 0x4000: { // 4xkk: skip next instruction if Vx != kk
			if (regs[(instruction & 0x0F00) >> 8] != (instruction & 0x00FF)) pc += 2;
		} break;

		case 0x5000: { // 5xy0: skip next instruction if Vx = Vy
			if (regs[(instruction & 0x0F00) >> 8] == regs[(instruction & 0x00F0) >> 4]) pc += 2;
		} break;

		case 0x6000: { // 6xkk: set Vx = kk
			regs[(instruction & 0x0F00) >> 8] = (instruction & 0x00FF);
		} break;

		case 0x7000: { // 7xkk: set Vx = Vx + kk
			regs[(instruction & 0x0F00) >> 8] += (instruction & 0x00FF);
		} break;

		case 0x8000: { // 8xy*
			byte x = (instruction & 0x0F00) >> 8;
			byte y = (instruction & 0x00F0) >> 4;
			switch (instruction & 0x000F) {
			case 0x0: // 8xy0: set Vx = Vy
				regs[x] = regs[y];
				break;
			case 0x1: // 8xy1: set Vx = Vx OR Vy
				regs[x] |= regs[y];
				regs[0xF] = 0;
				break;
			case 0x2: // 8xy2: set Vx = Vx AND Vy
				regs[x] &= regs[y];
				regs[0xF] = 0;
				break;
			case 0x3: // 8xy3: set Vx = Vx XOR Vy
				regs[x] ^= regs[y];
				regs[0xF] = 0;
				break;
			case 0x4: { // 8xy4: set Vx = Vx + Vy, set VF = carry
				word sum = regs[x] + regs[y];
				regs[x] = sum & 0xFF;
				regs[0xF] = (sum > 0xFF) ? 1 : 0;
			} break;
			case 0x5: {// 8xy5: set Vx = Vx - Vy, set VF = NOT borrow
				byte originalX = regs[x];
				regs[x] -= regs[y];
				regs[0xF] = (originalX >= regs[y]) ? 1 : 0;
			} break;
			case 0x6: {// 8xy6: set Vx = Vx SHR 1
				byte lsb = regs[y] & 0x1;
				regs[x] = regs[y];
				regs[x] >>= 1;
				regs[0xF] = lsb;
			} break;
			case 0x7: { // 8xy7: set Vx = Vy - Vx, set VF = NOT borrow
				byte originalX = regs[x];
				regs[x] = regs[y] - regs[x];
				regs[0xF] = (regs[y] >= originalX) ? 1 : 0;
			} break;
			case 0xE: { // 8xyE: set Vx = Vx SHL 1
				byte msb = (regs[y] & 0x80) >> 7;
				regs[x] = regs[y];
				regs[x] <<= 1;
				regs[0xF] = msb;
			} break;
			}
		} break;

		case 0x9000: { // 9xy0: skip next insruction if Vx != Vy
			if ((regs[(instruction & 0x0F00) >> 8]) != (regs[(instruction & 0x00F0) >> 4])) pc += 2;
		} break;

		case 0xA000: { // Annn: set i = nnn
			iReg = instruction & 0x0FFF;
		} break;

		case 0xB000: { // Bnnn: jump to location nnn + V0
			pc = (instruction & 0x0FFF) + regs[0];
		} break;

		case 0xC000: { // Cxkk: set Vx = random byte AND kk
			std::mt19937 rng(static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
			std::uniform_int_distribution<int> dist(0, 255);
			regs[(instruction & 0x0F00) >> 8] = dist(rng) & (instruction & 0x00FF);
		} break;

		case 0xD000: { // Dxyn: display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
			byte x = regs[(instruction & 0x0F00) >> 8];
			byte y = regs[(instruction & 0x00F0) >> 4];
			byte n = instruction & 0x000F;
			regs[0xF] = 0;

			for (int row = 0; row < n; row++) {
				if ((y % HEIGHT) + row >= HEIGHT) break;
				byte spriteByte = mem[iReg + row];
				for (int col = 0; col < 8; col++) {
					if ((x % WIDTH) + col >= WIDTH) break;
					byte pixel = (spriteByte >> (7 - col)) & 0x1;
					byte screenX = (x % WIDTH) + col;
					byte screenY = (y % HEIGHT) + row;
					if (pixel == 1) {
						if (screen[screenY][screenX] == 1) regs[0xF] = 1;
						screen[screenY][screenX] ^= 1;
					}
				}
			}

		} break;
		
		case 0xE000: { // Ex**
			byte x = (instruction & 0x0F00) >> 8;
			switch (instruction & 0x00FF) {
			case 0x9E: // Ex9E: skip next instruction if key with the value of Vx is pressed
				if (input[regs[x]]) pc += 2;
				break;
			case 0xA1: // ExA1: skip next instruction if key with the value of Vx is not pressed
				if (!input[regs[x]]) pc += 2;
				break;
			}
		} break;
		
		case 0xF000: { // Fx**
			byte x = (instruction & 0x0F00) >> 8;
			switch (instruction & 0x00FF) {
			case 0x07: // Fx07: set Vx = delay timer value
				regs[x] = delayReg;
				break;
			case 0x0A:  // Fx0A: wait for a key press, store the value of the key in Vx
				tempReg = x;
				c8keState = PAUSED;
				break;
			case 0x15: // Fx15: set delay timer = Vx
				delayReg = regs[x];
				break;
			case 0x18: // Fx18: set sound timer = Vx
				soundReg = regs[x];
				break;
			case 0x1E: // Fx1E: set i = i + Vx
				iReg += regs[x];
				break;
			case 0x29: // Fx29: set i = location of sprite for digit Vx
				iReg = SPRITE_ADDRESS + (regs[(instruction & 0x0F00) >> 8] * 5);
				break;
			case 0x33: { // Fx33: store BCD representation of Vx in memory locations i, i+1, and i+2
				byte number = regs[(instruction & 0x0F00) >> 8];
				mem[iReg] = number / 100;
				mem[iReg + 1] = (number / 10) % 10;
				mem[iReg + 2] = number % 10;
			} break;
			case 0x55: { // Fx55: store registers V0 through Vx in memory starting at location i
				byte x = (instruction & 0x0F00) >> 8;
				for (int i = 0; i <= x; i++) { mem[iReg] = regs[i]; iReg++; }
			} break;
			case 0x65: { // Fx65: read registers V0 through Vx from memory starting at location i
				byte x = (instruction & 0x0F00) >> 8;
				for (int i = 0; i <= x; i++) { regs[i] = mem[iReg]; iReg++; }
			} break;
			}
		} break;
		}
	}

};


void init() {
	// try to initialize SDL video and audio subsystems
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
		SDL_Log("SDL could not initialize. SDL error: %s\n", SDL_GetError());
		exit(1);
	}

	// try to intialize main window
	window = SDL_CreateWindow("SDL3 window", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
	if (window == nullptr) { SDL_Log("SDL could not init window. SDL error: %s\n", SDL_GetError()); exit(1); }

	// try to initialize main renderer on main window
	renderer = SDL_CreateRenderer(window, NULL);
	if (renderer == nullptr) { SDL_Log("SDL could not init renderer. SDL error: %s\n", SDL_GetError()); exit(1); }

	SDL_zero(e); // zero out SDL events structure

	// try to initialize audio
	stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
	if (!stream) { SDL_Log("SDL could not init audio stream: %s", SDL_GetError()); exit(1); }
	SDL_ResumeAudioStreamDevice(stream);
}


void events(c8ke emu) {
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_EVENT_QUIT) { c8keState = QUIT; return; }

		if (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP) {
			std::unordered_map<SDL_Keycode, byte>::iterator key = keymap.find(e.key.key);
			if (key != keymap.end()) {
				bool value = (e.type == SDL_EVENT_KEY_DOWN) ? true : false;
				input[key->second] = value;

				if (c8keState == PAUSED && !value) {
					c8keState = RUNNING;
					emu.regs[tempReg] = key->second;
				}
			}
		}

	}
}


void draw() {
	// prepare draw buffer
	SDL_FRect rects[WIDTH * HEIGHT];
	int rectCount = 0;
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			if (screen[y][x]) {
				rects[rectCount] = SDL_FRect{
					static_cast<float>(x * SCALE),
					static_cast<float>(y * SCALE),
					static_cast<float>(SCALE),
					static_cast<float>(SCALE),
				};
				rectCount++;
			}
		}
	}

	// clear and draw to buffer
	SDL_SetRenderDrawColor(renderer, br, bg, bb, ba);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, fr, fg, fb, fa);
	SDL_RenderFillRects(renderer, rects, rectCount);
	SDL_RenderPresent(renderer);
}


void beep(bool beep) {
	if (!beep) { SDL_ClearAudioStream(stream); return; }

	const int total = SDL_min(beepAmount / sizeof(float), 128); // how many float samples to generate (100 bytes worth, capped at 128 samples)
	float samples[128];  // Array to hold generated audio samples

	for (int i = 0; i < total; i++) {
		float gain = (i < 32) ? (float)i / 32.0f : 1.0f; // volume gradually increases over the first 32 samples
		float phase = current_sine_sample++ * beepPhase / 8000.0f; // calculate the current phase of the sine wave (2200 Hz tone, 8000 Hz sample rate)
		samples[i] = SDL_sinf(phase * 2.0f * SDL_PI_F) * gain; // generate sine wave sample and apply the gain
	}

	current_sine_sample %= 8000; // prevent the sine sample index from growing too large over time
	SDL_PutAudioStreamData(stream, samples, total * sizeof(float)); // queue the generated samples to the audio stream
}


void run(c8ke emu) {
	const double TIME_PER_CYCLE = 1000000000 / CLK;
	const double TIME_PER_REFRESH = 1000000000 / FPS;
	double cycleDelta = 0.0, refreshDelta = 0.0;
	long long elapsed = 0;
	std::chrono::high_resolution_clock::time_point now, last;

	last = std::chrono::high_resolution_clock::now();
	while (c8keState != QUIT) { // main loop
		now = std::chrono::high_resolution_clock::now();
		elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - last).count();
		cycleDelta += elapsed;
		refreshDelta += elapsed;

		// handles events, input
		events(emu);

		// cycle instructions
		while (cycleDelta >= TIME_PER_CYCLE) {
			cycleDelta -= TIME_PER_CYCLE;
			if (c8keState != PAUSED) emu.cycle();
		}

		// update screen, sound, delay
		if (refreshDelta >= TIME_PER_REFRESH) {
			refreshDelta -= TIME_PER_REFRESH;
			draw();

			if (emu.delayReg > 0) emu.delayReg--;
			if (emu.soundReg > 0) emu.soundReg--;

		}

		// actual sound
		beep((emu.soundReg > 0) ? true : false);

		last = now;
	}

}


void shutdown() {
	SDL_DestroyAudioStream(stream);
	stream = nullptr;

	SDL_DestroyRenderer(renderer);
	renderer = nullptr;

	SDL_DestroyWindow(window);
	window = nullptr;

	SDL_Quit();
}


int main(int argc, char* args[]) {
	c8ke c8ke;
	c8ke.reset();

	init();
	run(c8ke);
	shutdown();

	return 0;
}
