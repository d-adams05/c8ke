#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <chrono>
#include <random>
#include <ios>
#include <unordered_map>
#include <cstring>

#include "SDL3/SDL.h" // v3.2.16
#include "SDL3/SDL_main.h" // v3.2.16
#include "SDL3_image/SDL_image.h" // v3.2.4
#include "ImGui/imgui.h" // v1.92.0
#include "ImGui/imgui_impl_sdl3.h" // v1.92.0
#include "ImGui/imgui_impl_sdlrenderer3.h" // v1.92.0
#include "ImGui/imgui_internal.h" // v1.92.0
#include "tinyfiledialogs/tinyfiledialogs.h" // v3.19.1

#include "c8ke.h"



/***** helper functions *****/

static void* Settings_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name) {
	if (strcmp(name, "Colors") == 0)
		return &customColors;
	if (strcmp(name, "Audio") == 0)
		return &customAudio;
	return nullptr;
}

static void Settings_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void* user_data, const char* line) {
	CustomColors* colorSettings = (CustomColors*)user_data;
	float r, g, b, a;

	if (sscanf_s(line, "emuFg=%f,%f,%f,%f", &r, &g, &b, &a) == 4)
		colorSettings->emuFg = ImVec4(r, g, b, a);
	else if (sscanf_s(line, "emuBg=%f,%f,%f,%f", &r, &g, &b, &a) == 4)
		colorSettings->emuBg = ImVec4(r, g, b, a);
	else if (sscanf_s(line, "dbgColor1=%f,%f,%f,%f", &r, &g, &b, &a) == 4)
		colorSettings->dbgColor1 = ImVec4(r, g, b, a);
	else if (sscanf_s(line, "dbgColor2=%f,%f,%f,%f", &r, &g, &b, &a) == 4)
		colorSettings->dbgColor2 = ImVec4(r, g, b, a);
	else if (sscanf_s(line, "dbgColor3=%f,%f,%f,%f", &r, &g, &b, &a) == 4)
		colorSettings->dbgColor3 = ImVec4(r, g, b, a);
	else if (sscanf_s(line, "dbgBg=%f,%f,%f,%f", &r, &g, &b, &a) == 4)
		colorSettings->dbgBg = ImVec4(r, g, b, a);
	else if (sscanf_s(line, "dbgHeaderFg=%f,%f,%f,%f", &r, &g, &b, &a) == 4)
		colorSettings->dbgHeaderFg = ImVec4(r, g, b, a);
	else if (sscanf_s(line, "dbgHeaderBg=%f,%f,%f,%f", &r, &g, &b, &a) == 4)
		colorSettings->dbgHeaderBg = ImVec4(r, g, b, a);

	CustomAudio* audioSettings = (CustomAudio*)user_data;
	int val;

	if (sscanf_s(line, "beepAmount=%d", &val) == 1)
		audioSettings->beepAmount = val;
	else if (sscanf_s(line, "beepPhase=%d", &val) == 1)
		audioSettings->beepPhase = val;

}

static void Settings_WriteAll(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* out_buf) {
	const CustomColors& c = customColors;
	out_buf->appendf("[%s][Colors]\n", handler->TypeName);
	out_buf->appendf("emuFg=%.3f,%.3f,%.3f,%.3f\n", c.emuFg.x, c.emuFg.y, c.emuFg.z, c.emuFg.w);
	out_buf->appendf("emuBg=%.3f,%.3f,%.3f,%.3f\n", c.emuBg.x, c.emuBg.y, c.emuBg.z, c.emuBg.w);
	out_buf->appendf("dbgColor1=%.3f,%.3f,%.3f,%.3f\n", c.dbgColor1.x, c.dbgColor1.y, c.dbgColor1.z, c.dbgColor1.w);
	out_buf->appendf("dbgColor2=%.3f,%.3f,%.3f,%.3f\n", c.dbgColor2.x, c.dbgColor2.y, c.dbgColor2.z, c.dbgColor2.w);
	out_buf->appendf("dbgColor3=%.3f,%.3f,%.3f,%.3f\n", c.dbgColor3.x, c.dbgColor3.y, c.dbgColor3.z, c.dbgColor3.w);
	out_buf->appendf("dbgBg=%.3f,%.3f,%.3f,%.3f\n", c.dbgBg.x, c.dbgBg.y, c.dbgBg.z, c.dbgBg.w);
	out_buf->appendf("dbgHeaderFg=%.3f,%.3f,%.3f,%.3f\n", c.dbgHeaderFg.x, c.dbgHeaderFg.y, c.dbgHeaderFg.z, c.dbgHeaderFg.w);
	out_buf->appendf("dbgHeaderBg=%.3f,%.3f,%.3f,%.3f\n", c.dbgHeaderBg.x, c.dbgHeaderBg.y, c.dbgHeaderBg.z, c.dbgHeaderBg.w);

	out_buf->appendf("\n");

	const CustomAudio& a = customAudio;
	out_buf->appendf("[%s][Audio]\n", handler->TypeName);
	out_buf->appendf("beepAmount=%d\n", a.beepAmount);
	out_buf->appendf("beepPhase=%d\n", a.beepPhase);
}

SDL_Keycode findSDLKeycode(byte chip8Key) {
	for (const auto& [keycode, val] : keymap) {
		if (val == chip8Key)
			return keycode;
	}
	return SDLK_UNKNOWN;
}

void clear() {
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			screen[y][x] = 0;
		}
	}
}



/***** emulator core *****/

struct c8ke {
	word instruction{}; // current instruction
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
		std::memset(mem, 0, sizeof(mem));

		// load sprites into memory
		for (int i = 0; i < TOTAL_SPRITE_SIZE; i++) {
			mem[SPRITE_ADDRESS + i] = sprites[i];
		}

		// signal in init state
		c8keState = INIT;
	}

	void loadRom(std::string path) {
		std::ifstream rom(path, std::ios::binary); // open file in binary mode
		if (!rom.is_open()) {
			std::cerr << "c8ke - Error opening rom file" << std::endl;
			exit(1);
		}

		byte byte;
		while (rom.read(reinterpret_cast<char*>(&byte), sizeof(byte))) { // read each line until there are no more lines
			mem[pc++] = byte;
		}

		pc = START_ADDRESS;
		rom.close();
		c8keState = RUNNING;
	}

	void cycle() {
		instruction = (mem[pc] << 8) | mem[pc + 1];
		pc += 2;

		switch (instruction & 0xF000) { // checks the first nibble
		case 0x0000: { // 00E*
			switch (instruction & 0x000F) {
			case 0x0: // 00E0: clear the display
				clear();
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
				c8keState = HALT;
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



/***** main functions *****/

void init() {
	/*****   initialize SDL components   *****/
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
		SDL_Log("SDL could not initialize. SDL error: %s\n", SDL_GetError());
		exit(1);
	}

	// main window
	window = SDL_CreateWindow("c8ke", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_HIGH_PIXEL_DENSITY);
	if (window == nullptr) { SDL_Log("SDL could not initialize window. SDL error: %s\n", SDL_GetError()); exit(1); }
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	// main renderer
	renderer = SDL_CreateRenderer(window, nullptr);
	if (renderer == nullptr) { SDL_Log("SDL could not initialize renderer. SDL error: %s\n", SDL_GetError()); exit(1); }

	// main texture
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WIDTH, HEIGHT);
	if (texture == nullptr) { SDL_Log("SDL could not initialize main texture: %s", SDL_GetError()); exit(1); }
	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

	// emulator icon
	icon = IMG_Load("res/cake.ico");
	if (icon == nullptr) { SDL_Log("SDL could not load icon: %s", SDL_GetError()); exit(1); }
	SDL_SetWindowIcon(window, icon);

	// audio
	stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
	if (stream == nullptr) { SDL_Log("SDL could not initialize audio stream: %s", SDL_GetError()); exit(1); }
	SDL_ResumeAudioStreamDevice(stream);

	// clear SDL events
	SDL_zero(e);

	/*****   initialize ImGui components   *****/
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// load custom font
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	myFont = io.Fonts->AddFontFromFileTTF("res/RobotoMono-Regular.ttf", 20.0f);
	if (myFont == nullptr) { SDL_Log("SDL could not load font: %s", SDL_GetError()); exit(1); }

	// setup handler for saving custom color settings
	ImGuiSettingsHandler handler;
	handler.TypeName = "Custom_Settings";
	handler.TypeHash = ImHashStr("Custom_Settings");
	handler.ReadOpenFn = Settings_ReadOpen;
	handler.ReadLineFn = Settings_ReadLine;
	handler.WriteAllFn = Settings_WriteAll;
	ImGui::GetCurrentContext()->SettingsHandlers.push_back(handler);

	// setup platform/renderer backends
	ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer3_Init(renderer);
}

void beep(bool beep) {
	if (!beep) { SDL_ClearAudioStream(stream); return; }

	const int total = SDL_min(customAudio.beepAmount / sizeof(float), 128); // how many float samples to generate (100 bytes worth, capped at 128 samples)
	float samples[128];  // Array to hold generated audio samples

	for (int i = 0; i < total; i++) {
		float gain = (i < 32) ? (float)i / 32.0f : 1.0f; // volume gradually increases over the first 32 samples
		float phase = current_sine_sample++ * customAudio.beepPhase / 8000.0f; // calculate the current phase of the sine wave (2200 Hz tone, 8000 Hz sample rate)
		samples[i] = SDL_sinf(phase * 2.0f * SDL_PI_F) * gain; // generate sine wave sample and apply the gain
	}

	current_sine_sample %= 8000; // prevent the sine sample index from growing too large over time
	SDL_PutAudioStreamData(stream, samples, total * sizeof(float)); // queue the generated samples to the audio stream
}

void events(c8ke& emu) {
	while (SDL_PollEvent(&e)) {

		ImGui_ImplSDL3_ProcessEvent(&e);

		// handle quitting
		if (e.type == SDL_EVENT_QUIT) {
			c8keState = QUIT;
			return;
		}

		// handle pausing
		if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_P) {
			c8keState = (c8keState == RUNNING) ? PAUSED : RUNNING;
			return;
		}

		// handle all other input
		if (e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP) {
			auto key = keymap.find(e.key.key);
			if (key != keymap.end()) {
				bool pressed = (e.type == SDL_EVENT_KEY_DOWN);
				input[key->second] = pressed;

				if (c8keState == HALT && !pressed) {
					emu.regs[tempReg] = key->second;
					c8keState = RUNNING;
				}
			}
		}
	}

}

void draw(c8ke& emu) {
	/***** ImGui *****/
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	// top menu bar
	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, IM_COL32(0, 0, 0, 255)); // white
	ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(60, 60, 60, 255)); // gray
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(60, 60, 60, 255)); // gray
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(60, 60, 60, 255)); // gray
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open", nullptr)) {
				char const* filterPatterns[1] = { "*.ch8" };
				char* openFileName = tinyfd_openFileDialog("Choose a CHIP-8 rom file to open", nullptr, 1, filterPatterns, "CH8 File", 1);
				if (openFileName) {
					romPath = openFileName;
					std::replace(romPath.begin(), romPath.end(), '\\', '/');
					c8keState = RELOAD;
				} else if (romPath.empty()) {
					c8keState = INIT;
				} else if (c8keState == HALT) {
					c8keState = DELAY_HALT;
				} else {
					c8keState = DELAYED;
				}
			} ImGui::Separator();

			if (ImGui::MenuItem("Reset", nullptr)) {
				if (!romPath.empty()) c8keState = RELOAD;
			}ImGui::Separator();

			if (ImGui::MenuItem("Close", nullptr)) {
				c8keState = RESET;
			}ImGui::Separator();

			if (ImGui::MenuItem("Quit", nullptr)) {
				c8keState = QUIT;
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Settings")) {
			if (ImGui::BeginMenu("Colors")) {
				if (ImGui::ColorButton("##emuFg", customColors.emuFg, COLOR_EDIT_FLAGS_COLOR_BUTTON, ImVec2(20, 20))) showFgPicker = !showFgPicker;
				ImGui::SameLine();
				ImGui::Text("Emulator Fg Color");
				if (showFgPicker) {
					ImGui::SameLine(0, 10);
					ImGui::ColorPicker4("##emuFgPicker", (float*)&customColors.emuFg, COLOR_EDIT_FLAGS_PICKER);
				}
				ImGui::Separator();

				if (ImGui::ColorButton("##emuBg", customColors.emuBg, COLOR_EDIT_FLAGS_COLOR_BUTTON, ImVec2(20, 20))) showBgPicker = !showBgPicker;
				ImGui::SameLine();
				ImGui::Text("Emulator Bg Color");
				if (showBgPicker) {
					ImGui::SameLine(0, 10);
					ImGui::ColorPicker4("##emuBgPicker", (float*)&customColors.emuBg, COLOR_EDIT_FLAGS_PICKER);
				}
				ImGui::Separator();

				if (ImGui::ColorButton("##dbgColor1", customColors.dbgColor1, COLOR_EDIT_FLAGS_COLOR_BUTTON, ImVec2(20, 20))) showDbgColor1Picker = !showDbgColor1Picker;
				ImGui::SameLine();
				ImGui::Text("DBG: Fg Color 1");
				if (showDbgColor1Picker) {
					ImGui::SameLine(0, 10);
					ImGui::ColorPicker4("##dbgColor1Picker", (float*)&customColors.dbgColor1, COLOR_EDIT_FLAGS_PICKER);
				}
				ImGui::Separator();

				if (ImGui::ColorButton("##dbgColor2", customColors.dbgColor2, COLOR_EDIT_FLAGS_COLOR_BUTTON, ImVec2(20, 20))) showDbgColor2Picker = !showDbgColor2Picker;
				ImGui::SameLine();
				ImGui::Text("DBG: Fg Color 2");
				if (showDbgColor2Picker) {
					ImGui::SameLine(0, 10);
					ImGui::ColorPicker4("##dbgColor2Picker", (float*)&customColors.dbgColor2, COLOR_EDIT_FLAGS_PICKER);
				}
				ImGui::Separator();

				if (ImGui::ColorButton("##dbgColor3", customColors.dbgColor3, COLOR_EDIT_FLAGS_COLOR_BUTTON, ImVec2(20, 20))) showDbgColor3Picker = !showDbgColor3Picker;
				ImGui::SameLine();
				ImGui::Text("DBG: Fg Color 3");
				if (showDbgColor3Picker) {
					ImGui::SameLine(0, 10);
					ImGui::ColorPicker4("##dbgColor3Picker", (float*)&customColors.dbgColor3, COLOR_EDIT_FLAGS_PICKER);
				}
				ImGui::Separator();

				if (ImGui::ColorButton("##dbgBg", customColors.dbgBg, COLOR_EDIT_FLAGS_COLOR_BUTTON, ImVec2(20, 20))) showDbgBgPicker = !showDbgBgPicker;
				ImGui::SameLine();
				ImGui::Text("DBG: Bg Color");
				if (showDbgBgPicker) {
					ImGui::SameLine(0, 10);
					ImGui::ColorPicker4("##dbgBgColor", (float*)&customColors.dbgBg, COLOR_EDIT_FLAGS_PICKER);
				}
				ImGui::Separator();

				if (ImGui::ColorButton("##dbgHeaderFg", customColors.dbgHeaderFg, COLOR_EDIT_FLAGS_COLOR_BUTTON, ImVec2(20, 20))) showDbgHeaderFgPicker = !showDbgHeaderFgPicker;
				ImGui::SameLine();
				ImGui::Text("DBG: Header Fg Color");
				if (showDbgHeaderFgPicker) {
					ImGui::SameLine(0, 10);
					ImGui::ColorPicker4("##dbgHeaderFgPicker", (float*)&customColors.dbgHeaderFg, COLOR_EDIT_FLAGS_PICKER);
				}
				ImGui::Separator();

				if (ImGui::ColorButton("##dbgHeaderBg", customColors.dbgHeaderBg, COLOR_EDIT_FLAGS_PICKER, ImVec2(20, 20))) showDbgHeaderBgPicker = !showDbgHeaderBgPicker;
				ImGui::SameLine();
				ImGui::Text("DBG: Header Bg Color");
				if (showDbgHeaderBgPicker) {
					ImGui::SameLine(0, 10);
					ImGui::ColorPicker4("##dbgHeaderBgPicker", (float*)&customColors.dbgHeaderBg, COLOR_EDIT_FLAGS_PICKER);
				}
				ImGui::Separator();

				if (ImGui::MenuItem("Reset to default")) {
					customColors.emuFg = DEFAULT_EMULATOR_FG;
					customColors.emuBg = DEFAULT_EMULATOR_BG;
					customColors.dbgColor1 = DEFAULT_DEBUG_COLOR_1;
					customColors.dbgColor2 = DEFAULT_DEBUG_COLOR_2;
					customColors.dbgColor3 = DEFAULT_DEBUG_COLOR_3;
					customColors.dbgBg = DEFAULT_DEBUG_BG;
					customColors.dbgHeaderFg = DEFAULT_DEBUG_HEADER_FG;
					customColors.dbgHeaderBg = DEFAULT_DEBUG_HEADER_BG;
				}

				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (ImGui::BeginMenu("Audio")) {
				ImGui::SetNextItemWidth(150);
				ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(19, 19, 19, 255));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(60, 60, 60, 255));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(60, 60, 60, 255));
				ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(19, 19, 19, 255));
				ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, IM_COL32(60, 60, 60, 255));
				ImGui::InputInt("Beep Adjustment 1", &customAudio.beepAmount);
				ImGui::PopStyleColor(5);
				ImGui::Separator();

				ImGui::SetNextItemWidth(150);
				ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(19, 19, 19, 255));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(60, 60, 60, 255));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(60, 60, 60, 255));
				ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(19, 19, 19, 255));
				ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, IM_COL32(60, 60, 60, 255));
				ImGui::InputInt("Beep Adjustment 2", &customAudio.beepPhase);
				ImGui::PopStyleColor(5);
				ImGui::Separator();

				if (ImGui::MenuItem("Reset to default")) {
					customAudio.beepAmount = DEFAULT_BEEP_AMOUNT;
					customAudio.beepPhase = DEFAULT_BEEP_PHASE;
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		} else {
			showFgPicker = false;
			showBgPicker = false;
			showDbgColor1Picker = false;
			showDbgColor2Picker = false;
			showDbgColor3Picker = false;
			showDbgBgPicker = false;
			showDbgHeaderFgPicker = false;
			showDbgHeaderBgPicker = false;
		}

		ImGui::EndMainMenuBar();
	}
	ImGui::PopStyleColor(4);

	// emulator screen
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
	ImGui::SetNextWindowSize(ImVec2(WIDTH * SCALE, HEIGHT * SCALE));
	ImGui::Begin("CHIP-8 Screen", nullptr, emulatorScreen);
	ImGui::Image((ImTextureID)texture, ImVec2(texture->w * SCALE, texture->h * SCALE));
	ImVec2 chip8_screen_pos = ImGui::GetWindowPos();
	ImVec2 chip8_screen_size = ImGui::GetWindowSize();
	ImGui::End();
	ImGui::PopStyleVar(2);

	// controls
	ImGui::PushStyleColor(ImGuiCol_Text, customColors.dbgHeaderFg);
	ImGui::PushStyleColor(ImGuiCol_TitleBg, customColors.dbgHeaderBg);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, customColors.dbgBg);
	ImGui::SetNextWindowPos(ImVec2(chip8_screen_pos.x + chip8_screen_size.x, chip8_screen_pos.y));
	ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH - chip8_screen_size.x, chip8_screen_size.y / 2 - 15));
	ImGui::Begin("Controls", nullptr, nonscrollable);

	for (int row = 0; row < 4; ++row) {
		for (int col = 0; col < 4; ++col) { // chip8 keys
			byte chip8Key = chip8Keys[row][col];
			ImGui::TextColored(customColors.dbgColor2, "%X ", chip8Key);
			ImGui::SameLine();
		}
		
		ImGui::TextColored(customColors.dbgColor1, " < = >  ");
		ImGui::SameLine();
		
		for (int col = 0; col < 4; ++col) { // mapped keys
			byte chip8Key = chip8Keys[row][col];
			SDL_Keycode keycode = findSDLKeycode(chip8Key);
			const char* keyName = (keycode != SDLK_UNKNOWN) ? SDL_GetKeyName(keycode) : "?";
			ImGui::TextColored(customColors.dbgColor2, "%s ", keyName);
			if (col < 3) ImGui::SameLine();
		}
	}

	const char* text = "Pause [P]";
	float windowWidth = ImGui::GetWindowSize().x;
	float textWidth = ImGui::CalcTextSize(text).x;
	ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
	ImGui::TextColored(customColors.dbgColor2, text);
	ImGui::End();
	ImGui::PopStyleColor(3);

	// main registers
	ImGui::PushStyleColor(ImGuiCol_Text, customColors.dbgHeaderFg);
	ImGui::PushStyleColor(ImGuiCol_TitleBg, customColors.dbgHeaderBg);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, customColors.dbgBg);
	ImGui::SetNextWindowPos(ImVec2(chip8_screen_pos.x + chip8_screen_size.x, chip8_screen_pos.y + (chip8_screen_size.y / 2) - 15));
	ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH - chip8_screen_size.x, chip8_screen_size.y / 2 + 15));
	ImGui::Begin("Main Registers", nullptr, nonscrollable);
	ImVec4 currentColor;

	ImGui::TextColored(customColors.dbgColor1, "Program Counter");
	ImGui::SameLine();
	currentColor = (emu.pc == 0) ? customColors.dbgColor3 : customColors.dbgColor2;
	ImGui::TextColored(currentColor, "\t%04X", emu.pc);

	ImGui::TextColored(customColors.dbgColor1, "Stack Pointer  ");
	ImGui::SameLine();
	currentColor = (emu.sp == 0) ? customColors.dbgColor3 : customColors.dbgColor2;
	ImGui::TextColored(currentColor, "\t%04X", emu.sp);

	ImGui::TextColored(customColors.dbgColor1, "Cur Instruction");
	ImGui::SameLine();
	currentColor = (emu.instruction == 0) ? customColors.dbgColor3 : customColors.dbgColor2;
	ImGui::TextColored(currentColor, "\t%04X", emu.instruction);

	ImGui::TextColored(customColors.dbgColor1, "Index Pointer  ");
	ImGui::SameLine();
	currentColor = (emu.iReg == 0) ? customColors.dbgColor3 : customColors.dbgColor2;
	ImGui::TextColored(currentColor, "\t%04X", emu.iReg);

	ImGui::TextColored(customColors.dbgColor1, "Delay Timer    ");
	ImGui::SameLine();
	currentColor = (emu.delayReg == 0) ? customColors.dbgColor3 : customColors.dbgColor2;
	ImGui::TextColored(currentColor, "\t%04X", emu.delayReg);

	ImGui::TextColored(customColors.dbgColor1, "Sound Timer    ");
	ImGui::SameLine();
	currentColor = (emu.soundReg == 0) ? customColors.dbgColor3 : customColors.dbgColor2;
	ImGui::TextColored(currentColor, "\t%04X", emu.soundReg);

	ImGui::End();
	ImGui::PopStyleColor(3);

	// general registers
	ImGui::PushStyleColor(ImGuiCol_Text, customColors.dbgHeaderFg);
	ImGui::PushStyleColor(ImGuiCol_TitleBg, customColors.dbgHeaderBg);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, customColors.dbgBg);
	ImGui::SetNextWindowPos(ImVec2(chip8_screen_pos.x + chip8_screen_size.x, chip8_screen_pos.y + chip8_screen_size.y));
	ImGui::SetNextWindowSize(ImVec2((WINDOW_WIDTH - chip8_screen_size.x) / 2, WINDOW_HEIGHT - chip8_screen_size.y - ImGui::GetFrameHeight()));
	ImGui::Begin("Registers", nullptr, nonscrollable);
	for (int i = 0; i < 16; i++) {
		ImGui::TextColored(customColors.dbgColor1, "0x%01X", i);
		ImGui::SameLine();
		currentColor = (emu.regs[i] == 0) ? customColors.dbgColor3 : customColors.dbgColor2;
		ImGui::TextColored(currentColor, "%04X", emu.regs[i]);
	}
	ImGui::End();
	ImGui::PopStyleColor(3);

	// stack
	ImGui::PushStyleColor(ImGuiCol_Text, customColors.dbgHeaderFg);
	ImGui::PushStyleColor(ImGuiCol_TitleBg, customColors.dbgHeaderBg);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, customColors.dbgBg);
	ImGui::SetNextWindowPos(ImVec2(chip8_screen_pos.x + chip8_screen_size.x + ((WINDOW_WIDTH - chip8_screen_size.x) / 2), chip8_screen_pos.y + chip8_screen_size.y));
	ImGui::SetNextWindowSize(ImVec2((WINDOW_WIDTH - chip8_screen_size.x) / 2, WINDOW_HEIGHT - chip8_screen_size.y - ImGui::GetFrameHeight()));
	ImGui::Begin("Stack", nullptr, nonscrollable);
	for (int i = 0; i < 16; i++) {
		ImGui::TextColored(customColors.dbgColor1, "0x%01X", i);
		ImGui::SameLine();
		currentColor = (emu.stack[i] == 0) ? customColors.dbgColor3 : customColors.dbgColor2;
		ImGui::TextColored(currentColor, "%04X", emu.stack[i]);
	}
	ImGui::End();
	ImGui::PopStyleColor(3);

	// memory
	ImGui::PushStyleColor(ImGuiCol_Text, customColors.dbgHeaderFg);
	ImGui::PushStyleColor(ImGuiCol_TitleBg, customColors.dbgHeaderBg);
	ImGui::PushStyleColor(ImGuiCol_TitleBgActive, customColors.dbgHeaderBg);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, customColors.dbgBg);
	ImGui::SetNextWindowPos(ImVec2(chip8_screen_pos.x, chip8_screen_pos.y + chip8_screen_size.y));
	ImGui::SetNextWindowSize(ImVec2(chip8_screen_size.x, WINDOW_HEIGHT - chip8_screen_size.y - ImGui::GetFrameHeight()));
	ImGui::Begin("Memory", nullptr, scrollable);
	bool byte2 = false;

	for (int i = 0; i < MAX_MEM; i += 16) {
		ImGui::TextColored(customColors.dbgColor1, "0x%04X\t", i);
		ImGui::SameLine();

		for (int j = 0; j < 16; j++) {
			unsigned char byte = emu.mem[i + j];
			currentColor = (byte == 0) ? customColors.dbgColor3 : customColors.dbgColor2;
			ImGui::TextColored(currentColor, "%02X", byte);
			ImGui::SameLine();

			if (byte2 && j < 15) {
				ImGui::Text(" ");
				ImGui::SameLine();
			}
			byte2 = !byte2;
		}

		ImGui::NewLine();
	}

	ImGui::End();
	ImGui::PopStyleColor(4);

	/***** SDL *****/
	Uint8 fr = (Uint8)(customColors.emuFg.x * 255.0f);
	Uint8 fg = (Uint8)(customColors.emuFg.y * 255.0f);
	Uint8 fb = (Uint8)(customColors.emuFg.z * 255.0f);
	Uint8 fa = (Uint8)(customColors.emuFg.w * 255.0f);
	Uint8 br = (Uint8)(customColors.emuBg.x * 255.0f);
	Uint8 bg = (Uint8)(customColors.emuBg.y * 255.0f);
	Uint8 bb = (Uint8)(customColors.emuBg.z * 255.0f);
	Uint8 ba = (Uint8)(customColors.emuBg.w * 255.0f);

	// draw the emulator screen
	SDL_SetRenderTarget(renderer, texture);
	SDL_SetRenderDrawColor(renderer, br, bg, bb, ba);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, fr, fg, fb, fa);
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			if (screen[y][x]) {
				SDL_FRect pixel = { (float)x, (float)y, 1, 1 }; // render 1x1 pixels
				SDL_RenderFillRect(renderer, &pixel);
			}
		}
	}
	SDL_SetRenderTarget(renderer, nullptr);


	/***** render and present *****/
	ImGui::Render();
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
	SDL_RenderPresent(renderer);
}

void run(c8ke& emu) {
	double cycleDelta = 0.0, refreshDelta = 0.0;
	long long elapsed = 0;
	std::chrono::high_resolution_clock::time_point now, last;

	last = std::chrono::high_resolution_clock::now();
	while (c8keState != QUIT) { // main loop

		// reset loaded rom
		if (c8keState == RELOAD) {
			clear();
			emu.reset();
			emu.loadRom(romPath);

			c8keState = RUNNING;
			cycleDelta = 0.0;
			refreshDelta = 0.0;
			last = std::chrono::high_resolution_clock::now();
		}

		// reset timing so emulator does not over-compensate
		if (c8keState == DELAYED) {
			cycleDelta = 0.0;
			refreshDelta = 0.0;
			last = std::chrono::high_resolution_clock::now();
			c8keState = RUNNING;
		}

		// completely reset the emulator, except for custom colors
		if (c8keState == RESET) {
			clear();
			emu.reset();
			romPath = "";

			c8keState = INIT;
			cycleDelta = 0.0;
			refreshDelta = 0.0;
			last = std::chrono::high_resolution_clock::now();

			emu.instruction = 0;
		}

		// setup for CHIP-8 halt instruction
		if (c8keState == DELAY_HALT) {
			cycleDelta = 0.0;
			refreshDelta = 0.0;
			last = std::chrono::high_resolution_clock::now();
			c8keState = HALT;
		}

		// calculate new timings
		now = std::chrono::high_resolution_clock::now();
		elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - last).count();
		cycleDelta += elapsed;
		refreshDelta += elapsed;

		// handles events, input
		events(emu);

		// cycle instructions
		while (cycleDelta >= TIME_PER_CYCLE) {
			cycleDelta -= TIME_PER_CYCLE;
			if (c8keState == RUNNING) emu.cycle();
		}

		// update screen, sound, delay
		if (refreshDelta >= TIME_PER_REFRESH) {
			refreshDelta -= TIME_PER_REFRESH;
			draw(emu);

			if (emu.delayReg > 0) emu.delayReg--;
			if (emu.soundReg > 0) emu.soundReg--;
		}

		// actual sound
		beep(emu.soundReg > 0);

		// update for next cycle
		last = now;
	}

}

void shutdown() {
	// safely shutdown ImGui
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	// safely shutdown SDL
	if (stream) { SDL_DestroyAudioStream(stream); stream = nullptr; }
	if (icon) { SDL_DestroySurface(icon); icon = nullptr; }
	if (texture) { SDL_DestroyTexture(texture); texture = nullptr; }
	if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
	if (window) { SDL_DestroyWindow(window); window = nullptr; }
	SDL_Quit();
}

int main(int argc, char* args[]) {
	c8ke emu;
	emu.reset();

	init();
	run(emu);
	shutdown();

	return 0;
}