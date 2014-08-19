#pragma once
#ifndef __CORE_H__
#define __CORE_H__

#include <cstdint>
#include <array>
#include <stack>
#include <memory>

using std::uint8_t;
using std::uint16_t;

#include "SDLManager.h"

class Core
{
private:	
	std::array < uint8_t, 4096 > memory;
	std::array < uint8_t, 16 > registers;
	std::array < bool, 64 * 32 > graphics;

	static const std::array < uint8_t, 80 > fontset;

	uint16_t index_register;
	uint16_t program_counter;

	uint8_t delay_timer;
	uint8_t sound_timer;

	std::stack < uint16_t > program_stack;

	bool empty;
	bool draw_flag;
	bool running;

	void executeOpcode(uint16_t opcode);

	std::unique_ptr <SDLManager> sdl;

public:
	Core();
	
	void load_ROM(std::string filename);

	bool isPressed(int key);

	void step_cycle();
	void emulate();

};

#endif