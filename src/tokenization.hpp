#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <optional>

enum class TokenType {
	exit,
	print,
	int_lit,
	let,
	ident,
	eq,
	open_paren,
	close_paren,
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

	std::vector<Token> tokenize()
	{
		std::vector<Token> tokens;
		std::string buf;
		while (peek().has_value()) {
			std::cout << "peek: " << peek().value() << " at index: " << m_index << std::endl;
			if (std::isalpha(peek().value())) {
				buf.push_back(consume());
				while (peek().has_value() && std::isalnum(peek().value())) {
					buf.push_back(consume());
				}
				if (buf == "exit") {
					tokens.push_back({ TokenType::exit });
					buf.clear();
				} else if (buf == "let") {
					tokens.push_back({ TokenType::let });
					buf.clear();
				} else {
					tokens.push_back({ TokenType::ident, buf });
					buf.clear();
				}
			} else if (std::isdigit(peek().value())) {
				buf.push_back(consume());
				while (peek().has_value() && std::isdigit(peek().value())) {
					buf.push_back(consume());
				}
				tokens.push_back({ TokenType::int_lit, buf });
				buf.clear();
			} else if (peek().value() == '(') {
				consume();
				tokens.push_back({ TokenType::open_paren });
			} else if (peek().value() == ')') {
				consume();
				tokens.push_back({ TokenType::close_paren });
			} else if (peek().value() == ';') {
				consume();
				tokens.push_back({ TokenType::semi });
			} else if (peek().value() == '=') {
				consume();
				tokens.push_back({ TokenType::eq });
			} else if (peek().value() == '\n') {
				consume();
			} else if (peek().value() == '\r') {
				consume();
			} else if (std::isspace(peek().value())) {
				consume();
			} else if (std::iscntrl(peek().value()) || !std::isprint(peek().value())) {
				std::cerr << "Ignoring non-printable character " << consume() << std::endl;
			} else {
				std::cerr << "Invalid token " << peek().value() << " at index " << m_index << std::endl;
				exit(EXIT_FAILURE);
			}
		}
		m_index = 0;
		return tokens;
	}
private:

	[[nodiscard]] inline std::optional<char> peek(int offset = 0) const {
		if (m_index + offset >= static_cast<int>(m_src.length())) {
			return {}; // Renvoie une option vide si l'indice est hors limites
		}
		return m_src.at(m_index + offset);
	}

	inline char consume() {
		if (m_index >= static_cast<int>(m_src.length())) {
			std::cerr << "Attempt to consume beyond end of source!" << std::endl;
			exit(EXIT_FAILURE);
		}
		return m_src.at(m_index++);
	}

	const std::string& m_src;
	int m_index = 0;
};