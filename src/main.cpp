#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

#include "./tokenization.hpp"
#include "./parser.hpp"
#include "./generation.hpp"

int main(int argc, char** argv)
{
	if (argc != 2) {
		std::cerr << "Usage : ./build/mine <input.me>" << std::endl;
		return EXIT_FAILURE;
	}

	std::string contents ;
	{
		std::stringstream contents_stream;
		std::fstream input(argv[1], std::ios::in);
		contents_stream << input.rdbuf();
		contents = contents_stream.str();
	}

	std::cout << contents << std::endl << std::endl;

	Tokenizer tokenizer(std::move(contents));
	std::vector<Token> tokens = tokenizer.tokenize();

	// Parser parser(std::move(tokens));
	// std::optional<NodeProg> prog = parser.parse_prog();

	// if (!prog.has_value()) {
	// 	std::cerr << "Unable to parse" << std::endl;
	// 	return EXIT_FAILURE;
	// }
	// Generator generator(prog.value());
	// std::string generated_asm = generator.gen_prog();

	// std::cout << generated_asm << std::endl << std::endl;

	// {
	// 	std::fstream file("./output/out.asm", std::ios::out);
	// 	file << generated_asm;
	// }



	// system("nasm -felf64 -o output/out.o output/out.asm");
	// system("ld output/out.o -o output/out");

	// system("./output/out ; echo $?");

	// To run the program
	// cmake --build build/ && echo && ./build/mine ./main.me

	return EXIT_SUCCESS;
}