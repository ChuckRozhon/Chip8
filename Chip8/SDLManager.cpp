#include "SDLManager.h"

SDLManager::SDLManager()
{
	initSDL();
	initGFX();
	initInput();
}

/* This is the definition of the Chip8 keyboard used:

Keypad                   Keyboard
+-+-+-+-+                +-+-+-+-+
|1|2|3|C|                |1|2|3|4|
+-+-+-+-+                +-+-+-+-+
|4|5|6|D|                |Q|W|E|R|
+-+-+-+-+       =>       +-+-+-+-+
|7|8|9|E|                |A|S|D|F|
+-+-+-+-+                +-+-+-+-+
|A|0|B|F|                |Z|X|C|V|
+-+-+-+-+                +-+-+-+-+
*/
std::array <int, 16> SDLManager::chip8_to_keyboard =
{
	SDL_SCANCODE_X, // 0
	SDL_SCANCODE_1, // 1
	SDL_SCANCODE_2, // 2
	SDL_SCANCODE_3, // 3
	SDL_SCANCODE_Q, // 4
	SDL_SCANCODE_W, // 5
	SDL_SCANCODE_E, // 6
	SDL_SCANCODE_A, // 7
	SDL_SCANCODE_S, // 8
	SDL_SCANCODE_D, // 9
	SDL_SCANCODE_Z, // A
	SDL_SCANCODE_C, // B
	SDL_SCANCODE_4, // C
	SDL_SCANCODE_R, // D
	SDL_SCANCODE_F, // E
	SDL_SCANCODE_V  // F
};

std::map <int, int> SDLManager::keyboard_to_chip8 =
{
	{ SDL_SCANCODE_X, 0 },
	{ SDL_SCANCODE_1, 1 },
	{ SDL_SCANCODE_2, 2 },
	{ SDL_SCANCODE_3, 3 },
	{ SDL_SCANCODE_Q, 4 },
	{ SDL_SCANCODE_W, 5 },
	{ SDL_SCANCODE_E, 6 },
	{ SDL_SCANCODE_A, 7 },
	{ SDL_SCANCODE_S, 8 },
	{ SDL_SCANCODE_D, 9 },
	{ SDL_SCANCODE_Z, 0xA },
	{ SDL_SCANCODE_C, 0xB },
	{ SDL_SCANCODE_4, 0xC },
	{ SDL_SCANCODE_R, 0xD },
	{ SDL_SCANCODE_F, 0xE },
	{ SDL_SCANCODE_V, 0xF }
};

int SDLManager::get_ticks()
{
	return SDL_GetTicks();
}

void SDLManager::sleep(long sleep_time)
{
	SDL_Delay(sleep_time);
}

bool SDLManager::isPressed(int chip8_key)
{
	const uint8_t * keys = SDL_GetKeyboardState(NULL);
	SDL_PumpEvents();
	return keys[chip8_to_keyboard[chip8_key]] != 0;
}

/* Waits for a valid key to be pressed (based off the keyboard array), then
returns the value of that key. */
int SDLManager::wait_for_press()
{
	int event_available = SDL_PollEvent(eventQueue.get());
	while (!event_available || eventQueue->type != SDL_KEYDOWN || keyboard_to_chip8.find(eventQueue->key.keysym.scancode) == keyboard_to_chip8.end())
	{
		SDL_PollEvent(eventQueue.get());
	}
	return keyboard_to_chip8[eventQueue->key.keysym.scancode];
}

void SDLManager::clearScreen()
{
	SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255);
	SDL_RenderClear(renderer.get());

	SDL_RenderPresent(renderer.get());
}

void SDLManager::initSDL()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		std::string error = SDL_GetError();
	}
}

bool SDLManager::checkOSEvents()
{
	const bool CLOSE = true;
	const bool OPEN = false;
	if (SDL_PollEvent(eventQueue.get()) == 1)
	{
		if (eventQueue->type == SDL_WINDOWEVENT)
		{
			switch (eventQueue->window.event)
			{
			case SDL_WINDOWEVENT_RESIZED:
			{
				resize_factor = SDL_min(eventQueue->window.data1 / 64, eventQueue->window.data2 / 32);
			} break;
			case SDL_WINDOWEVENT_CLOSE:
			{
				return CLOSE;
			} break;
			default:
				return OPEN;
			}
		}
	}
	return OPEN;
}

void SDLManager::initGFX()
{
	window = std::unique_ptr<SDL_Window, SDLWindowDeleter>(SDL_CreateWindow(
		"Chip8",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		64,
		32,
		SDL_WINDOW_RESIZABLE
		));

	renderer = std::unique_ptr<SDL_Renderer, SDLRendererDeleter>(SDL_CreateRenderer(
		window.get(),
		-1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE
		));

	surface = std::unique_ptr<SDL_Surface, SDLSurfaceDeleter>(SDL_CreateRGBSurface(
		0,
		64,
		32,
		32,
		0xFF000000,
		0x00FF0000,
		0x0000FF00,
		0x000000FF));
	
	texture = std::unique_ptr<SDL_Texture, SDLTextureDeleter>(SDL_CreateTexture(
		renderer.get(),
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_STREAMING,
		64,
		32));

	SDL_SetSurfaceRLE(surface.get(), 1);

	SDL_RenderSetLogicalSize(renderer.get(), 64, 32);
}

void SDLManager::initInput()
{
	eventQueue = std::unique_ptr <SDL_Event>(new SDL_Event);
}

SDLManager::~SDLManager()
{
	SDL_Quit();
}