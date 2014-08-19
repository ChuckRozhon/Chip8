#include "Core.h"

#include <iostream>
#include <fstream>
#include <cassert>
#include <random>
#include <cstdlib>

const std::array < uint8_t, 80 > Core::fontset =
{
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
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Core::Core()
{
	program_counter = 0x200;
	index_register = 0;
	empty = false;
	draw_flag = false;

	memory.fill(0);

	for (size_t i = 0; i < fontset.size(); ++i)
	{
		memory[i] = fontset[i];
	}

	sdl = std::unique_ptr<SDLManager>(new SDLManager());
	sdl->clearScreen();

	empty = true;
}

void Core::step_cycle()
{
	// Check program counter for the current opcode
	uint16_t currentOpcode = memory[program_counter] << 8 | memory[program_counter + 1];

	// Execute the current opcode
	executeOpcode(currentOpcode);

	// Increase the program counter
	program_counter += 2;

	if (sound_timer > 0)
	{
		if (sound_timer == 1) std::cout << "BEEP" << std::endl;
		sound_timer --;
	}
}

void Core::emulate()
{
	running = true;

	const int FPS = 60;
	const int SKIP_TICKS = 1000 / FPS;

	int sleep_time = 0;
	int next_game_tick = sdl->get_ticks();

	while (running)
	{
		for (size_t i = 0; i < 10; i++)
		{
			step_cycle(); // Executes about 10 cycles per draw call
		}

		if (draw_flag)
		{
			sdl->draw_array(graphics);
		}
		draw_flag = false;

		// Check for screen updates or closure
		if (sdl->checkOSEvents())
		{
			running = false;
		}

		next_game_tick += SKIP_TICKS;
		sleep_time = next_game_tick - sdl->get_ticks();

		if (delay_timer > 0)
		{
			delay_timer--;
		}
		if (sleep_time >= 0)
		{
			sdl->sleep(sleep_time);
		}
	}
}

void Core::load_ROM(std::string filename)
{
	std::streampos size;
	uint8_t * memory_ptr = memory.data() + 0x200;

	std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
	if (file.is_open())
	{
		size = file.tellg();
		file.seekg(0, std::ios::beg);
		file.read((char*)memory_ptr, size);
		file.close();

		std::cout << "File: " << filename << " has been loaded into memory. " << std::endl;
		empty = false;
	}
	else
	{
		std::cout << "Unable to open file: " << filename << std::endl;
	}
}

bool Core::isPressed(int key)
{
	return sdl->isPressed(key);
}

void Core::executeOpcode(uint16_t opcode)
{
	uint16_t address = opcode & 0x0FFF;

	size_t x = (opcode & 0x0F00) >> 8;
	size_t y = (opcode & 0x00F0) >> 4;

	uint8_t kk = (opcode & 0x00FF);

	uint8_t randByte;

	switch (opcode & 0xF000)
	{
	case 0x0000:
	{
		switch (opcode)
		{
		/* 0x0000: EXT - Signifies the end of a program. */
		case 0x0000:
		{
			running = false;
			break;
		}

		/* 0x00E0: CLS - Clear the display */
		case 0x00E0:
		{
			graphics.fill(false);
			break;
		}

		/* 0x00EE: RET - Returns from a subroutine by setting the program 
		   counter to the address at the top of the stack. */
		case 0x00EE:
		{
			program_counter = program_stack.top();
			program_stack.pop();
			break;
		}

		default:
			break;
		}
		break;
	}

	/* 0x1NNN: JP addr - Sets the program counter to NNN */
	case 0x1000:
	{
		program_counter = address;
		program_counter -= 2;
		break;
	}

	/* 0x2NNN: CALL addr - Puts the current program counter on the top of the stack.
	   The program counter is then set to NNN. */
	case 0x2000:
	{
		program_stack.push(program_counter);
		program_counter = address;
		program_counter -= 2;
		break;
	}

	/* 0x3XKK: SE VX - Compares register X to KK, and if they are equal, increments the 
	   program counter by 2, skipping the next instruction. */
	case 0x3000:
	{
		if (registers[x] == kk)
		{
			program_counter += 2;
		}
		break;
	}

	/* 0x4XKK: SNE VX - Compares register X to KK, and if they are NOT equal, increments
	   program counter by 2, skipping the next instruction. */
	case 0x4000:
	{
		if (registers[x] != kk)
		{
			program_counter += 2;
		}
		break;
	}

	/* 0x5XY0: SE VX, VY - Compares register X to register Y, and if they are equal, 
	   increments program counter by 2. */
	case 0x5000:
	{
		if (registers[x] == registers[y])
		{
			program_counter += 2;
		}
		break;
	}

	/* 0x6XKK: LD Vx, byte - Set VX = KK. Puts the value of KK into register X. */
	case 0x6000:
	{
		registers[x] = kk;
		break;
	}

	/* 0x7XKK: ADD Vx, byte - Set VX = VX + KK. Adds the value of KK to the 
	   value of register X. */
	case 0x7000:
	{
		registers[x] += kk;
		break;
	}

	/* 0x8XYN: Arithmetic opcodes */
	case 0x8000:
	{
		switch (opcode & 0x000F)
		{
		/* 0x8XY0: LD Vx, Vy - Set VX = VY. Stores the value of register Y in
		   register X. */
		case 0x0000:
		{
			registers[x] = registers[y];
			break;
		}

		/* 0x8XY1: OR Vx, Vy - Set VX = VX OR VY. Performs a bitwise OR on the
		   values in register X and register Y and places the value in register
		   X. */
		case 0x0001:
		{
			registers[x] = registers[x] | registers[y];
			break;
		}

		/* 0x8XY2: AND Vx, Vy - Set VX = VX AND VY. Performs a bitwise AND on 
		   the values in register X and register Y and places the value in 
		   register X. */
		case 0x0002:
		{
			registers[x] = registers[x] & registers[y];
			break;
		}

		/* 0x8XY3: XOR Vx, Vy - Set VX = VX XOR VY. Performs a bitwise XOR on 
		   the values in register X and register Y and places the value in 
		   register X. */
		case 0x0003:
		{
			registers[x] = registers[x] ^ registers[y];
			break;
		}

		/* 0x8XY4: ADD Vx, Vy - The values of register X and register Y are 
		   added together. If the result is greater than 8 bits, then register
		   0xF (15) is set to 1, otherwise 0. Only the last 8 bits of the result
		   are kept and stored in register X. */
		case 0x0004:
		{
			bool willOverflow = UINT8_MAX - registers[x] < registers[y];

			if (willOverflow)
			{
				registers[0xF] = 1;
				registers[x] = ((uint16_t)registers[x] + (uint16_t)registers[y]) & 0xFF;
			}
			else
			{
				registers[0xF] = 0;
				registers[x] = registers[x] + registers[y];
			}
			break;
		}

		/* 0x8XY5: SUB Vx, Vy - If Vx < Vy, then register 0xF (15) is set to 1,
		   otherwise 0. Then register y is subtracted from register x and the 
		   result is stored in register x. */
		case 0x0005:
		{
			if (registers[x] < registers[y])
			{
				registers[0xF] = 0;
				uint8_t subtract = registers[y];
				subtract = subtract - registers[x] - 1;
				registers[x] = UINT8_MAX - subtract;
			}
			else
			{
				registers[0xF] = 1;
				registers[x] = registers[x] - registers[y];
			}
			break;
		}

		/* 0x8XY6: SHR Vx - Shifts register x right by one. Register 0xF (15) is set to 
		   value of the least significant bit of register x before the shift. 
		   */
		case 0x0006:
		{
			registers[0xF] = registers[x] % 2;
			registers[x] = registers[x] >> 1;
			break;
		}

		/* 0x8XY7: SUBN Vx, Vy - If Vy > Vx, then VF is set to 1, otherwise 0. Then
		   Vx is subtracted from Vy, and the results stored in Vx. */
		case 0x0007:
		{
			if (registers[y] > registers[x])
			{
				registers[0xF] = 1;
				uint8_t subtract = registers[x];
				subtract = subtract - registers[y];
				registers[x] = UINT8_MAX - subtract;
			}
			else
			{
				registers[0xF] = 0;
				registers[x] = registers[y] - registers[x];
			}
			break;
		}

		/* 0x8XYE: SHL Vx -	Shifts register x to the left by one. If the most
		   significant bit of register x is 1, then VF is set to 1, otherwise 
		   0. */
		case 0x000E:
		{
			registers[0xF] = registers[x] % 2;
			registers[x] = registers[x] << 1;
			break;
		}

		default:
			break;
		}
		break;
	}

	/* 0x9XY0: SNE Vx, Vy - Skips next instruction if Register x != Register y. */
	case 0x9000:
	{
		if (registers[x] != registers[y])
		{
			program_counter += 2;
		}
		break;
	}

	/* 0xANNN: LD I, addr - Sets the index register to the value NNN. */
	case 0xA000:
	{
		index_register = address;
		break;
	}

	/* 0xBNNN: JP V0, addr - Sets the program counter to the address NNN + register 0. */
	case 0xB000:
	{
		program_counter = address + registers[0];
		program_counter -= 2;
		break;
	}

	/* 0xCXKK: RND Vx, byte - Sets VX = random byte AND KK. */
	case 0xC000:
	{
		std::random_device rd;
		std::default_random_engine generator(rd());
		std::uniform_int_distribution<int> distribution(0, UINT8_MAX);
		randByte = distribution(generator);

		registers[x] = randByte & kk;

		break;
	}

	/* 0xDXYN: DRW Vx, Vy, nibble - Display a sprite with width of 8 pixels and height of n pixels
	   starting at memory location I at (Vx, Vy), sets VF = collision. */
	case 0xD000:
	{
		uint8_t height = opcode & 0x000F;
		uint8_t pixel_line;	

		registers[0xF] = 0;

		for (int yline = 0; yline < height; ++yline)
		{
			pixel_line = memory[index_register + yline];
			for (int xline = 0; xline < 8; ++xline)
			{
				if ((pixel_line & (0x80 >> xline)) != 0)
				{
					int index = ((registers[x] + xline) + ((registers[y] + yline) * 64)) % 2048;
					registers[0xF] = graphics[index];
					graphics[index] ^= 1;
				}
			}
		}

		draw_flag = true;
		break;
	}

	case 0xE000:
	{
		switch(opcode & 0x00FF)
		{
		/* 0xEX9E: SKP Vx - Skips next instruction if key with value Vx is pressed.*/
		case 0x009E:
		{
			if (sdl->isPressed(registers[x]))
			{
				program_counter += 2;
			}
			break;
		}

		/* 0xEXA1: SKNP Vx - Skips next instruction if key with value Vx is not
		   pressed. */
		case 0x00A1:
		{
			if (! sdl->isPressed(registers[x]))
			{
				program_counter += 2;
			}
			break;
		}

		default:
			break;
		}

		break;
	}

	case 0xF000:
	{
		switch(opcode & 0x00FF)
		{
		/* 0xFX07: LD Vx, DT - Sets register x equal to the value of the delay timer. */
		case 0x0007:
		{
			registers[x] = delay_timer;	
			break;
		}
		
		/* 0xFX0A: LD Vx, K - Wait for a key press, store the value of the pressed
		   key in register x. */
		case 0x000A:
		{
			int pressed = sdl->wait_for_press();
			assert(pressed >= 0 && pressed <= 0xF);
			registers[x] = (uint8_t) pressed;
			break;
		}

		/* 0xFX15: LD DT, Vx - Sets the delay timer equal to the value of register x. */
		case 0x0015:
		{
			delay_timer = registers[x];
			break;
		}

		/* 0xFX18: LD ST, Vx - Sets the sound timer equal to the value of register x. */
		case 0x0018:
		{
			sound_timer = registers[x];
			break;
		}

		/* 0xFX1E: ADD I, Vx - Set I = I + register x. */
		case 0x001E:
		{
			index_register += registers[x];
			break;
		}

		/* 0xFX29: LD F, Vx - Set I = location of sprite for digit Vx. */
		case 0x0029:
		{
			index_register = registers[x] * 5;
			break;
		}

		/* 0xFX33: LD B, Vx - Store BCD representation of register x in memory
		   locations I, I + 1, and I + 2. */
		case 0x0033:
		{
			memory[index_register]     = registers[x] / 100;
  			memory[index_register + 1] = (registers[x] / 10) % 10;
 			memory[index_register + 2] = (registers[x] % 100) % 10;
			break;
		}

		/* 0xFX55: LD [I], Vx - Store registers V0 through Vx in memory starting 
		   at location I. */
		case 0x0055:
		{
			for (size_t i = 0; i < (x + 1); ++i)
			{
				memory[i + index_register] = registers[i];
			}
			break;
		}

		/* 0xFX65: LD Vx, [I] - Reads registers V0 through Vx from memory starting
		   at location I. */
		case 0x0065:
		{
			for (size_t i = 0; i < (x + 1); ++i)
			{
				registers[i] = memory[i + index_register];
			}
			break;
		}

		default:
			break;
		}
		break;
	}

	default:
	{
		std::cout << "Unregonized opcode: " << opcode << std::endl;
		std::cout << "Exiting" << std::endl;
		std::exit(EXIT_FAILURE);
		break;
	}
	}
}