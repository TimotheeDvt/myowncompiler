#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <optional>

enum class TokenType {
	exit,
	int_lit,
	semi
};

struct Token {
	TokenType type;
	std::optional<std::string> value;
};

class Tokenizer {
public:
	inline explicit Tokenizer(std::string src):
		m_src(std::move(src))
	{};

	inline std::vector<Token> tokenize() {
		std::vector<Token> tokens;
		for(int i = 0; i < m_src.length(); i++) {
			char c = m_src.at(i);

			std::string buf {};
			while(peek().has_value()) {
				if (std::isalpha(peek().value())) {
					buf.push_back(consume());
					while(peek().has_value() && std::isalnum(peek().value())) {
						buf.push_back(consume());
					}

					if (buf == "exit") {
						tokens.push_back(Token{TokenType::exit});
						buf.clear();
						continue;
					} else {
						std::cerr << "Unknown token : "<< buf << std::endl;
						exit(EXIT_FAILURE);
					}
				} else if (std::isdigit(peek().value())) {
					buf.push_back(consume());
					while(peek().has_value() && std::isdigit(peek().value())) {
						buf.push_back(consume());
					}
					tokens.push_back(Token{TokenType::int_lit, buf});
					buf.clear();
					continue;
				} else if (peek().value() == ';') {
					consume();
					tokens.push_back(Token{TokenType::semi});
					continue;
				} else if (std::isspace(peek().value())) {
					consume();
					continue;
				} else {
					std::cerr << "Unknown token "<< buf << std::endl;
					exit(EXIT_FAILURE);
				}
			}
		}

		m_index = 0;
		return tokens;
	};
private:

	[[nodiscard]] inline std::optional<char> peek(int offset = 1) const {
		if (m_index + offset > m_src.length()) {
			return {};
		}
		return m_src.at(m_index + offset - 1);
	}

	inline char consume() {
		return m_src.at(m_index++);
	}

	const std::string& m_src;
	size_t m_index = 0;
};