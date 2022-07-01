#include "types.h"
#include "memory.h"
#include "wad.h"
#include "system.h"
#include "map.h"
#include "renderer.h"
#include "vectors.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <stdio.h>

static f32 camerax, cameray;
static f32 zoom;

static SDL_Window* window = 0;

static Slice<char> titleBuffer = {};


int main(int argc, char** argv) {

	logMessage("Initializing memory");
	initMemory();

	logMessage("Initializing wad files");
	initWads();

	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVENTS) < 0) {
		fatalError("Failed to init SDL");
	}

	window = SDL_CreateWindow("Doom Node Visualizer - 0.01 alpha", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, 0);
	if(!window) {
		fatalError("Failed to open window");
	}

	titleBuffer.data = (char*)memoryAlloc(permanent, sizeof(char) * 1024);
	titleBuffer.length = 1024;

	bool isRunning = true;
	SDL_Event event;

	SDL_Surface* screen = SDL_GetWindowSurface(window);
	if(!screen) {
		fatalError("Failed to get window surface");
	}
	if(screen->format->BitsPerPixel != 32) {
		fatalError("Failed to set 32-bit color");
	}

	DrawContext drawContext = {
		screen->w,
		screen->h,
		screen->w / 2,
		screen->h / 2,
		screen->pitch,
		4,
		(u8*)screen->pixels,
		screen->format->Rshift, screen->format->Gshift, screen->format->Bshift,
		screen->format->Rmask, screen->format->Gmask, screen->format->Bmask
	};

	logMessage("Initializing renderer");
	initRenderer(drawContext);

	const u64 timeSamples = 10;
	u64 times[timeSamples] = {};
	i32 tick = 0;

	u64 frameStart = SDL_GetPerformanceCounter(), lastTime;
	u64 counterFreq = SDL_GetPerformanceFrequency();

	// @TODO: Load wad file from argv
	const i8* wadFilePath = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Doom 2\\base\\DOOM2.WAD";

	logMessage("Loading wad file %s...", wadFilePath);

	WadResult wadResult = loadWadFile(wadFilePath);
	if (wadResult == WadResult::Failure) {
		fatalError("Failed to load wad");
	}

	Array<LumpNum> mapLumps = findMapLumps();
	if (mapLumps.length == 0) {
		fatalError("Wad contains no map lumps");
	}

	i32 mapIndex = 0;
	MapLoad mapLoad = loadMap(mapLumps[mapIndex]);

	RenderState renderState = {
		mapLoad.map->nodes.length - 1,
		0
	};
	View view = calculateView(mapLoad.map, drawContext, renderState.selectedNode);

	while (isRunning) {
		lastTime = frameStart;
		frameStart = SDL_GetPerformanceCounter();

		f32 secondsElapsed = (f32)(frameStart - lastTime) / (f32)counterFreq;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT: {
					isRunning = false;
				} break;
				case SDL_KEYDOWN: {
					if (event.key.keysym.sym == SDLK_ESCAPE) {
						isRunning = false;
					}
				} break;
			}
		}

		if(SDL_LockSurface(screen) != 0) {
			fatalError("Failed to lock surface");
		}

		if (mapLoad.result == MapResult::Success) {
			i32 x, y;
			SDL_GetMouseState(&x, &y);

			f32 worldx = (x - drawContext.xcenter - view.offset.x) / view.zoom;
			f32 worldy = (drawContext.ycenter - y + view.offset.y) / view.zoom;

			renderState.highlightedSide = pointOnLineSide(worldx, worldy, mapLoad.map->nodes[renderState.selectedNode]);

			renderMap(mapLoad.map, view, drawContext, renderState);
		}
		else {
			// Show error to user?
		}

		SDL_UnlockSurface(screen);
		SDL_UpdateWindowSurface(window);

		resetArena(temporary);

		times[tick % timeSamples] = SDL_GetPerformanceCounter() - frameStart;

		if (tick % timeSamples == timeSamples - 1) {
			f64 totalTicks = 0;
			for (int i = 0; i < timeSamples; ++i) {
				totalTicks += times[i];
			}

			/*if (keyboard_state[SDL_SCANCODE_SPACE]) {
				f64 avgFrameTime = ((f64)totalTicks / timeSamples) * 1000;
				f64 avgMilliseconds = avgFrameTime / counterFreq;
				f64 avgFPS = 1000.0 / avgMilliseconds;
				logMessage("Average MS: %f (%f FPS)", avgFrameTime / counterFreq, avgFPS);
			}*/
		}

		++tick;
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	reportMemoryStats();

	return 0;
}
