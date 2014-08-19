#pragma once
#ifndef __SDLMANAGER_H__
#define __SDLMANAGER_H__

#include <memory>
#include <string>
#include <array>
#include <map>

#include "SDL.h"

class SDLManager
{
public:
	SDLManager();

	void clearScreen();

	bool isPressed(int key);
	int wait_for_press();

	int get_ticks();
	void sleep(long sleep_time);

	template <int size>
	void draw_array(std::array<bool, size>& in_array);

	bool checkOSEvents();

	~SDLManager();
private:
	struct SDLWindowDeleter
	{
		void operator()(SDL_Window * window)
		{
			SDL_DestroyWindow(window);
		}
	};
	struct SDLRendererDeleter
	{
		void operator()(SDL_Renderer * renderer)
		{
			SDL_DestroyRenderer(renderer);
		}
	};
	struct SDLSurfaceDeleter
	{
		void operator()(SDL_Surface * surface)
		{
			SDL_FreeSurface(surface);
		}
	};
	struct SDLTextureDeleter
	{
		void operator()(SDL_Texture * texture)
		{
			SDL_DestroyTexture(texture);
		}
	};

	int resize_factor;

	static std::array <int, 16> chip8_to_keyboard;
	static std::map <int, int> keyboard_to_chip8;

	void initSDL();
	void initGFX();
	void initInput();

	std::unique_ptr <SDL_Event> eventQueue;
	std::unique_ptr <SDL_Window, SDLWindowDeleter> window;
	std::unique_ptr <SDL_Renderer, SDLRendererDeleter> renderer;

	std::unique_ptr <SDL_Surface, SDLSurfaceDeleter> surface;
	std::unique_ptr <SDL_Texture, SDLTextureDeleter> texture;
};

template <int size>
void SDLManager::draw_array(std::array <bool, size>& in_array)
{
	SDL_LockTexture(texture.get(), NULL, &surface->pixels, &surface->pitch);

	SDL_LockSurface(surface.get());

	for (int x = 0; x < surface->w; ++x)
	{
		for (int y = 0; y < surface->h; ++y)
		{
			uint32_t* pixels = (uint32_t *)surface->pixels;
			int index = (y * surface->w) + x;
			if (in_array[index])
			{
				pixels[index] = 0xFFFFFFFF;
			}
			else
			{
				pixels[index] = 0;
			}
		}
	}
	SDL_UnlockSurface(surface.get());

	SDL_UnlockTexture(texture.get());

	SDL_RenderCopy(renderer.get(), texture.get(), NULL, NULL);

	SDL_RenderPresent(renderer.get());
}

#endif