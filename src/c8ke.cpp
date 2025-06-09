#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <random>
#include <ios>

#include <SDL3/SDL.h> // v3.2.16
#include <SDL3/SDL_main.h> // v3.2.16

using byte = unsigned char; // 8 bits, 1 bite
using word = unsigned short; // 16 bits, 2 bites

const unsigned short CLK = 500; // 500 Hz, 500 cycles/sec
const unsigned char FPS = 60; // 60 FPS, 60 frames/sec
const unsigned short MAX_MEM = 4096; // 4KB memory, 4096 bites
const unsigned short START = 0x200; // memory start address

constexpr int WINDOW_WIDTH = 640;
constexpr int WINDOW_HEIGHT = 320;

const int width = 64;
const int height = 32;
int screen[height][width] = { 0 };
bool input[16] = { 0 };

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_AudioStream* stream = nullptr;
SDL_AudioSpec spec;
int current_sine_sample = 0;
SDL_Event e;

enum State {
	INIT = -1,
	RUNNING = 0,
	QUIT = 1,
	PAUSED = 2,
	HALT = 3,
};
State c8keState = INIT;
int tempReg = 0;

//////////////////////////////////////////////////////////////////////////


void beep(bool power) {
	int amount = 100;
	if (power) {
		amount /= sizeof(float);  /* convert from bytes to samples */

		float samples[128];  /* feed 128 samples each iteration */
		const int total = SDL_min(amount, SDL_arraysize(samples));
		int i;

		/* generate a 440Hz pure tone with attack */
		for (i = 0; i < total; i++) {
			const int freq = 2200;
			const float phase = current_sine_sample * freq / 8000.0f;
			float value = SDL_sinf(phase * 2 * SDL_PI_F);

			// Apply attack: ramp up volume over first 32 samples
			const int attack_samples = 32;
			float gain = 1.0f;
			if (i < attack_samples) {
				gain = (float)i / (float)attack_samples;
			}

			samples[i] = value * gain;
			current_sine_sample++;
		}

		current_sine_sample %= 8000;

		SDL_PutAudioStreamData(stream, samples, total * sizeof(float));
	}
	else {
		SDL_ClearAudioStream(stream);
	}
}


struct c8ke {
	word pc{}; // 16-bit program counter
	byte sp{}; // 8-bit stack pointer

	word stack[16]{}; // 16 16-bit values
	byte regs[16]{}; // 16 8-bit registers
	byte mem[MAX_MEM]{}; // program memory

	word iReg{}; // 16-bit i register
	byte delayReg{}; // 8-bit delay timer register
	byte soundReg{}; // 8-bit sound timer register

	c8ke() {
		pc = START;
		sp = -1;
		iReg = 0;
		delayReg = 0;
		soundReg = 0;

		for (byte i = 0; i < 16; i++) {
			stack[i] = 0;
			regs[i] = 0;
		}

		//loadRom("res\\test\\1-chip8-logo.ch8"); // passed
		//loadRom("res\\test\\2-ibm-logo.ch8"); // passed
		//loadRom("res\\test\\3-corax+.ch8"); // passed
		//loadRom("res\\test\\4-flags.ch8"); // passed
		//loadRom("res\\test\\5-quirks.ch8"); // passed
		//loadRom("res\\test\\6-keypad.ch8"); // passed
		loadRom("res\\test\\7-beep.ch8"); // passed
		//loadRom("res\\games\\Tetris [Fran Dachille, 1991].ch8");


		int sprites[80] = {
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
		int count = 0;
		for (int i = 80; i < 160; i++) {
			mem[i] = sprites[i - 80];
		}


	}

	void loadRom(std::string path) {
		std::ifstream rom(path, std::ios::binary); // Open file in binary mode
		byte byte; // Loading the file byte by byte
		int temp = pc;
		
		if (!rom.is_open()) {
			std::cerr << "error opening file" << std::endl;
			exit(1);
		}
		
		while (rom.read(reinterpret_cast<char*>(&byte), sizeof(byte))) { // Read each line until there are no more lines
			mem[temp] = byte; // Reading a single byte at a time
			temp++;
		}

		rom.close();
	}

	void cycle() {
		word instruction = (mem[pc] << 8) | mem[pc + 1];
		pc += 2;

		//std::cout << std::hex << instruction << std::endl;

		switch (instruction & 0xF000) { // checks the first nibble
			case 0x0000:
				switch (instruction & 0x000F) {
					case 0x0: // 0x00E0: clear the display
						for (int y = 0; y < height; y++) {
							for (int x = 0; x < width; x++) {
								screen[y][x] = 0;
							}
						}
						break;
					case 0xE: // 0x00EE: return from a subroutine
						pc = stack[sp];
						sp--;
						break;
				}
				break;

			case 0x1000: // 0x1nnn: jump to location nnn
				pc = instruction & 0x0FFF;
				break;

			case 0x2000: // 0x2nnn: call subroutine at nnn
				sp++;
				stack[sp] = pc;
				pc = instruction & 0x0FFF;
				break;

			case 0x3000: // 0x3xkk: skip next instruction if Vx = kk
				if (regs[(instruction & 0x0F00) >> 8] == (instruction & 0x00FF)) pc += 2;
				break;

			case 0x4000: // 0x4xkk: skip next instruction if Vx != kk
				if (regs[(instruction & 0x0F00) >> 8] != (instruction & 0x00FF)) pc += 2;
				break;

			case 0x5000: // 0x5xy0: skip next instruction if Vx = Vy
				if ((instruction & 0x000F) != 0) break;
				if (regs[(instruction & 0x0F00) >> 8] == regs[(instruction & 0x00F0) >> 4]) pc += 2;
				break;

			case 0x6000: // 6xkk: set Vx = kk
				regs[(instruction & 0x0F00) >> 8] = (instruction & 0x00FF);
				break;

			case 0x7000: // 0x7xkk: set Vx = Vx + kk
				regs[(instruction & 0x0F00) >> 8] += (instruction & 0x00FF);
				break;

			case 0x8000: { // 0x8xy
				byte x = (instruction & 0x0F00) >> 8;
				byte y = (instruction & 0x00F0) >> 4;
				switch (instruction & 0x000F) {
					case 0x0: // 0x8xy0: set Vx = Vy
						regs[x] = regs[y];
						break;
					case 0x1: // 0x8xy1: set Vx = Vx OR Vy
						regs[x] |= regs[y];
						regs[0xF] = 0;
						break;
					case 0x2: // 0x8xy2: set Vx = Vx AND Vy
						regs[x] &= regs[y];
						regs[0xF] = 0;
						break;
					case 0x3: // 0x8xy3: set Vx = Vx XOR Vy
						regs[x] ^= regs[y];
						regs[0xF] = 0;
						break;
					case 0x4: { // 0x8xy4: set Vx = Vx + Vy, set VF = carry
						word sum = regs[x] + regs[y];
						regs[x] = sum & 0xFF;  // ensure 8-bit result
						regs[0xF] = (sum > 0xFF) ? 1 : 0;
					} break;
					case 0x5: {// 0x8xy5: set Vx = Vx - Vy, set VF = NOT borrow
						int ogX = regs[x];
						regs[x] -= regs[y];
						regs[0xF] = (ogX >= regs[y]) ? 1 : 0;
					} break;
					case 0x6: {// 0x8xy6: set Vx = Vx SHR 1
						byte leastSignificantBit = regs[y] & 0x1;
						regs[x] = regs[y];
						regs[x] >>= 1;
						regs[0xF] = leastSignificantBit;
					}break;
					case 0x7: { // 0x8xy7: set Vx = Vy - Vx, set VF = NOT borrow
						int ogX = regs[x];
						regs[x] = regs[y] - regs[x];
						regs[0xF] = (regs[y] >= ogX) ? 1 : 0;
					} break;
					case 0xE: { // 0x8xyE: set Vx = Vx SHL 1
						byte mostSignificantBit = (regs[y] & 0x80) >> 7;
						regs[x] = regs[y];
						regs[x] <<= 1;
						regs[0xF] = mostSignificantBit;
					} break;
				}
				break; }

			case 0x9000: // 0x9xy0: skip next insruction if Vx != Vy
				if ((regs[(instruction & 0x0F00) >> 8]) != (regs[(instruction & 0x00F0) >> 4])) pc += 2;
				break;

			case 0xA000: // 0xAnnn: set i = nnn
				iReg = instruction & 0x0FFF;
				break;

			case 0xB000: // 0xBnnn: jump to location nnn + V0
				pc = (instruction & 0x0FFF) + regs[0];
				break;

			case 0xC000: { // 0xCxkk: set Vx = random byte AND kk
				std::mt19937 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
				std::uniform_int_distribution<int> dist(0, 255);
				regs[(instruction & 0x0F00) >> 8] = dist(rng) & (instruction & 0x00FF);
				break; }

			case 0xD000: { // 0xDxyn: draw sprite at (Vx, Vy), n bytes tall
				int x = regs[(instruction & 0x0F00) >> 8];
				int y = regs[(instruction & 0x00F0) >> 4];
				int n = instruction & 0x000F;
				regs[0xF] = 0;


				for (int row = 0; row < n; row++) {

					if ((y % height) + row >= height) break;

					byte spriteByte = mem[iReg + row];

					for (int col = 0; col < 8; col++) {
						if ((x % width) + col >= width) break;

						int pixel = (spriteByte >> (7 - col)) & 0x1;
						int screenX = (x % width) + col;
						int screenY = (y % height) + row;

						if (pixel == 1) {
							if (screen[screenY][screenX] == 1) regs[0xF] = 1;
							screen[screenY][screenX] ^= 1;
						}
					}
				}


				break;
			}

			case 0xE000: {
				byte x = (instruction & 0x0F00) >> 8;
				switch (instruction & 0x00FF) {
					case 0x9E: // 0xEx9E: skip next instruction if key with the value of Vx is pressed
						if (input[regs[x]]) pc += 2;
						break;
					case 0xA1: // 0xExA1: skip next instruction if key with the value of Vx is not pressed
						if (!input[regs[x]]) pc += 2;
						break;
				}
				break; }

			case 0xF000: {
				byte x = (instruction & 0x0F00) >> 8;
				switch (instruction & 0x00FF) {
				case 0x07: // 0xFx07: set Vx = delay timer value
					regs[x] = delayReg;
					break;
				case 0x0A:  // 0xFx0A: wait for a key press, store the value of the key in Vx
					tempReg = x;
					c8keState = HALT;
					break;
				case 0x15: // 0xFx15: set delay timer = Vx
					delayReg = regs[x];
					break;
				case 0x18: // 0xFx18: set sound timer = Vx
					soundReg = regs[x];
					break;
				case 0x1E: // 0xFx1E: set i = i + Vx
					iReg += regs[x];
					break;
				case 0x29: // 0xFx29: set i = location of sprite for digit Vx
					iReg = 80 + (regs[(instruction & 0x0F00) >> 8] * 5);
					break;
				case 0x33: { // 0xFx33: store BCD representation of Vx in memory locations i, i+1, and i+2
					int number = regs[(instruction & 0x0F00) >> 8];
					mem[iReg] = number / 100;
					mem[iReg + 1] = (number / 10) % 10;
					mem[iReg + 2] = number % 10;
					break;
				}
				case 0x55: { // 0xFx55: store registers V0 through Vx in memory starting at location i
					int x = (instruction & 0x0F00) >> 8;
					for (int i = 0; i <= x; i++) { mem[iReg] = regs[i]; iReg++; }
					break;
				}
				case 0x65: { // 0xFx65: read registers V0 through Vx from memory starting at location i
					int x = (instruction & 0x0F00) >> 8;
					for (int i = 0; i <= x; i++) { regs[i] = mem[iReg]; iReg++; }
					break;
				}

				break;
				}
			}
		}
	}
};


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

		while (SDL_PollEvent(&e)) { // poll events
			if (e.type == SDL_EVENT_QUIT) {
				c8keState = QUIT;
			}
			else if (c8keState == HALT) {
				if (e.type == SDL_EVENT_KEY_UP) {
					switch (e.key.key) {
					case SDLK_1:
						input[1] = false;
						c8keState = RUNNING;
						emu.regs[tempReg] = 1;
						break;
					case SDLK_2:
						input[2] = false;
						c8keState = RUNNING;
						emu.regs[tempReg] = 2;
						break;
					case SDLK_3:
						input[3] = false;
						c8keState = RUNNING;
						emu.regs[tempReg] = 3;
						break;
					case SDLK_4:
						input[0xC] = false;
						c8keState = RUNNING;
						emu.regs[tempReg] = 0xC;
						break;
					case SDLK_Q:
						input[4] = false;
						c8keState = RUNNING;
						emu.regs[tempReg] = 4;
						break;
					case SDLK_W:
						input[5] = false;
						c8keState = RUNNING;
						emu.regs[tempReg] = 5;
						break;
					case SDLK_E:
						input[6] = false;
						c8keState = RUNNING;
						emu.regs[tempReg] = 6;
						break;
					case SDLK_R:
						input[0xD] = false;
						c8keState = RUNNING;
						emu.regs[tempReg] = 0xD;
						break;
					case SDLK_A:
						input[7] = false;
						c8keState = RUNNING;
						emu.regs[tempReg] = 7;
						break;
					case SDLK_S:
						input[8] = false;
						c8keState = RUNNING;
						emu.regs[tempReg] = 8;
						break;
					case SDLK_D:
						input[9] = false;
						c8keState = RUNNING;
						emu.regs[tempReg] = 9;
						break;
					case SDLK_F:
						input[0xE] = false;
						c8keState = RUNNING;
						emu.regs[tempReg] = 0xE;
						break;
					case SDLK_Z:
						input[0xA] = false;
						c8keState = RUNNING;
						emu.regs[tempReg] = 0xA;
						break;
					case SDLK_X:
						input[0] = false;
						c8keState = RUNNING;
						emu.regs[tempReg] = 0;
						break;
					case SDLK_C:
						input[0xB] = false;
						c8keState = RUNNING;
						emu.regs[tempReg] = 0xB;
						break;
					case SDLK_V:
						input[0xF] = false;
						c8keState = RUNNING;
						emu.regs[tempReg] = 0xF;
						break;
					}
				}
			} else if (e.type == SDL_EVENT_KEY_DOWN) {
				switch (e.key.key) {
					case SDLK_1:
						input[1] = true;
						break;
					case SDLK_2:
						input[2] = true;
						break;
					case SDLK_3:
						input[3] = true;
						break;
					case SDLK_4:
						input[0xC] = true;
						break;
					case SDLK_Q:
						input[4] = true;
						break;
					case SDLK_W:
						input[5] = true;
						break;
					case SDLK_E:
						input[6] = true;
						break;
					case SDLK_R:
						input[0xD] = true;
						break;
					case SDLK_A:
						input[7] = true;
						break;
					case SDLK_S:
						input[8] = true;
						break;
					case SDLK_D:
						input[9] = true;
						break;
					case SDLK_F:
						input[0xE] = true;
						break;
					case SDLK_Z:
						input[0xA] = true;
						break;
					case SDLK_X:
						input[0] = true;
						break;
					case SDLK_C:
						input[0xB] = true;
						break;
					case SDLK_V:
						input[0xF] = true;
						break;
				}
			} else if (e.type == SDL_EVENT_KEY_UP) {
				switch (e.key.key) {
				case SDLK_1:
					input[1] = false;
					break;
				case SDLK_2:
					input[2] = false;
					break;
				case SDLK_3:
					input[3] = false;
					break;
				case SDLK_4:
					input[0xC] = false;
					break;
				case SDLK_Q:
					input[4] = false;
					break;
				case SDLK_W:
					input[5] = false;
					break;
				case SDLK_E:
					input[6] = false;
					break;
				case SDLK_R:
					input[0xD] = false;
					break;
				case SDLK_A:
					input[7] = false;
					break;
				case SDLK_S:
					input[8] = false;
					break;
				case SDLK_D:
					input[9] = false;
					break;
				case SDLK_F:
					input[0xE] = false;
					break;
				case SDLK_Z:
					input[0xA] = false;
					break;
				case SDLK_X:
					input[0] = false;
					break;
				case SDLK_C:
					input[0xB] = false;
					break;
				case SDLK_V:
					input[0xF] = false;
					break;
				}
			}
		}

		while (cycleDelta >= TIME_PER_CYCLE) { // do stuff
			cycleDelta -= TIME_PER_CYCLE;
			if (c8keState != HALT) emu.cycle();

			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderClear(renderer);

			SDL_SetRenderDrawColor(renderer, 0xFA, 0xC8, 0x98, 255);

			SDL_FRect rects[width * height];
			int rectCount = 0;

			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					if (screen[y][x]) {
						rects[rectCount] = SDL_FRect{
							static_cast<float>(x * 10),
							static_cast<float>(y * 10),
							static_cast<float>(10),
							static_cast<float>(10),
						};
						rectCount++;
					}
				}
			}

			SDL_RenderFillRects(renderer, rects, rectCount);


		}

		if (refreshDelta >= TIME_PER_REFRESH) { // update screen
			refreshDelta -= TIME_PER_REFRESH;
			if (emu.delayReg > 0) emu.delayReg--;
			if (emu.soundReg > 0) emu.soundReg--;
			SDL_RenderPresent(renderer);
		}

		beep((emu.soundReg > 0) ? true : false);

		last = now;
	}

}


void init() {
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) { // try to initialize SDL video subsystem
		SDL_Log("SDL could not initialize. SDL error: %s\n", SDL_GetError()); exit(1);
	}

	if (window = SDL_CreateWindow("SDL3 window", WINDOW_WIDTH, WINDOW_HEIGHT, 0); window == nullptr) { // try to create main window
		SDL_Log("SDL could not create window. SDL error: %s\n", SDL_GetError()); exit(1);
	}

	if (renderer = SDL_CreateRenderer(window, NULL); renderer == nullptr) {
		SDL_Log("SDL could not create main renderer. SDL error: %s\n", SDL_GetError()); exit(1);
	}

	SDL_zero(e); // zero out SDL event structure

	spec.channels = 1;
	spec.format = SDL_AUDIO_F32;
	spec.freq = 8000;
	stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
	if (!stream) {
		SDL_Log("SDL could not create audio stream: %s", SDL_GetError());
		exit(1);
	}
	SDL_ResumeAudioStreamDevice(stream);
}


void shutdown() {
	SDL_DestroyRenderer(renderer);
	renderer = nullptr;

	SDL_DestroyWindow(window); // clean up window and main surface
	window = nullptr;

	SDL_Quit(); // close out SDL subsystems
}


int main(int argc, char* args[]) {
	c8ke c8ke; // emulator core, self initialized

	init(); // initialize SDL components
	run(c8ke); // run main core, this is the main loop
	shutdown(); // clean up SDL components

	return 0;
}
