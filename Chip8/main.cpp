#include "Core.h"
#include <iostream>

int main(int argc, char ** argv)
{
	if (argc < 2)
	{
		std::cout << "Usage: Chip8 'chip8 application' \n\n" << std::endl;
	}

	auto myChip8 = new Core();

	myChip8->load_ROM(argv[1]);

	myChip8->emulate();

	return 0;
}