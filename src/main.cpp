#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <vector>

#include "./generation.hpp"
#include "./arena.hpp"

int main(int argc, char* argv[])
{
	if (argc != 2) {
		std::cerr << "Incorrect usage. Correct usage is..." << std::endl;
		std::cerr << "mine <input.me>" << std::endl;
		return EXIT_FAILURE;
	}

	std::string contents;
	{
		std::stringstream contents_stream;
		std::fstream input(argv[1], std::ios::in);
		contents_stream << input.rdbuf();
		contents = contents_stream.str();
	}

	std::cout << contents << std::endl << std::endl;

	Tokenizer tokenizer(std::move(contents));
	std::vector<Token> tokens = tokenizer.tokenize();

	Parser parser(std::move(tokens));
	std::optional<NodeProg> prog = parser.parse_prog();

	if (!prog.has_value()) {
		std::cerr << "Invalid program" << std::endl;
		exit(EXIT_FAILURE);
	}

	Generator generator(prog.value());
	{
		std::fstream file("output/out.asm", std::ios::out);
		file << generator.gen_prog();
	}
	system("nasm -felf64 -o output/out.o output/out.asm");
	system("ld output/out.o -o output/out");

	std::cout << "Executing... " << std::endl;
	system("./output/out ; echo Exit code : $?");

	// To run the program
	// cmake --build build/ && echo && ./build/mine ./main.me

	return EXIT_SUCCESS;
}