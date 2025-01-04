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

std::vector<Token> tokenize(const std::string& str) {
	std::vector<Token> tokens;
	for(int i = 0; i < str.length(); i++) {
		char c = str.at(i);

		std::string buf {};
		if (isalpha(c)) {
			buf.push_back(c);
			++i;
			while(isalnum(str.at(i))) {
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
		} else if (isdigit(c)) {
			buf.push_back(c);
			++i;
			while(isdigit(str.at(i))) {
				buf.push_back(str.at(i));
				++i;
			}
			--i;
			
			tokens.push_back(Token{TokenType::int_lit, buf});
			buf.clear();
		} else if (c == ';') {
			tokens.push_back(Token{TokenType::semi});
		} else if (isspace(c)) {
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

	std::fstream input(argv[1], std::ios::in);

	std::string contents ;
	{
		std::stringstream contents_stream;
		contents_stream << input.rdbuf();
		input.close();
		contents = contents_stream.str();
	}

	std::cout << contents << std::endl;

	tokenize(contents);

	return EXIT_SUCCESS;
}