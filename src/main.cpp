#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

enum class TokenType {
	_exit,
	int_lit,
	semi
};

struct Token {
	TokenType type;
	std::optional<std::string> value;
};

std::stringstream tokens_to_asm(const std::vector<Token>& tokens) {
	std::stringstream output;
	output << "global _start\n_start:\n";

	for (int i = 0; i < tokens.size(); i++) {
		Token token = tokens.at(i);
		if (token.type == TokenType::_exit) {
			if (i+1 < tokens.size() && tokens.at(i+1).type == TokenType::int_lit) {
				if (i+2 < tokens.size() && tokens.at(i+2).type == TokenType::semi) {
					output << "    mov rax, 60\n";
					output << "    mov rdi, " << tokens.at(i+1).value.value() << "\n";
					output << "    syscall\n";
					i += 2;
				} else {
					std::cerr << "Expected ; after int literal" << std::endl;
					exit(EXIT_FAILURE);
				}
			} else {
				std::cerr << "Expected int literal after exit" << std::endl;
				exit(EXIT_FAILURE);
			}
		}
	}

	return output;
}

std::vector<Token> tokenize(const std::string& str) {
	std::vector<Token> tokens;
	for(int i = 0; i < str.length(); i++) {
		char c = str.at(i);

		std::string buf {};
		if (std::isalpha(c)) {
			buf.push_back(c);
			++i;
			while(std::isalnum(str.at(i))) {
				buf.push_back(str.at(i));
				++i;
			}
			--i;
			
			if (buf == "exit") {
				tokens.push_back(Token{TokenType::_exit});
				buf.clear();
			} else {
				std::cerr << "Unknown token "<< buf << std::endl;
				exit(EXIT_FAILURE);
			}
		} else if (std::isdigit(c)) {
			buf.push_back(c);
			++i;
			while(std::isdigit(str.at(i))) {
				buf.push_back(str.at(i));
				++i;
			}
			--i;
			
			tokens.push_back(Token{TokenType::int_lit, buf});
			buf.clear();
		} else if (c == ';') {
			tokens.push_back(Token{TokenType::semi});
		} else if (std::isspace(c)) {
			continue;
		} else {
			std::cerr << "Unknown token "<< buf << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	return tokens;
};


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

	std::vector<Token> tokens = tokenize(contents);

	std::cout << tokens_to_asm(tokens).str() << std::endl;

	{
		std::fstream file("./output/out.asm", std::ios::out);
		file << tokens_to_asm(tokens).str();
	}

	system("nasm -felf64 -o output/out.o output/out.asm");
	system("ld output/out.o -o output/out");

	system("./output/out ; echo $?");

	return EXIT_SUCCESS;
}