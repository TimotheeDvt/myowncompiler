#pragma once

#include <string>
#include <vector>

enum class TokenType {
	exit,
	int_lit,
	semi,
	open_paren,
	close_paren,
	ident,
	let,
	eq,
	plus,
	minus,
	star,
	fslash,
	print,
	open_brace,
	close_brace,
	_if,
	_for,
	from,
	to
};

struct Token {
	TokenType type;
	std::optional<std::string> value {};
};

std::optional<int> bin_prec(TokenType type) {
	switch (type) {
	case TokenType::plus:
	case TokenType::minus:
		return 0;
	case TokenType::star:
	case TokenType::fslash:
		return 1;
	default:
		return {};
	}
}

class Tokenizer {
public:
	inline explicit Tokenizer(std::string src)
		: m_src(std::move(src)) { }

	inline std::vector<Token> tokenize() {
		std::vector<Token> tokens;
		std::string buf;
		while (peek().has_value()) {
			if (std::isalpha(peek().value())) {
				buf.push_back(consume());
				while (peek().has_value() && std::isalnum(peek().value())) {
					buf.push_back(consume());
				}
				if (buf == "exit") {
					tokens.push_back(Token{TokenType::exit });
					buf.clear();
				} else if (buf == "let") {
					tokens.push_back(Token{TokenType::let });
					buf.clear();
				} else if (buf == "if") {
					tokens.push_back(Token{TokenType::_if });
					buf.clear();
				} else if (buf == "for") {
					tokens.push_back(Token{TokenType::_for });
					buf.clear();
				} else if (buf == "from") {
					tokens.push_back(Token{TokenType::from });
					buf.clear();
				} else if (buf == "to") {
					tokens.push_back(Token{TokenType::to });
					buf.clear();
				} else if (buf == "print") {
					tokens.push_back(Token{TokenType::print });
					buf.clear();
				} else {
					tokens.push_back(Token{TokenType::ident, buf });
					buf.clear();
				}
			} else if (std::isdigit(peek().value())) {
				buf.push_back(consume());
				while (peek().has_value() && std::isdigit(peek().value())) {
					buf.push_back(consume());
				}
				tokens.push_back(Token{TokenType::int_lit, buf });
				buf.clear();
			} else if (peek().value() == '(') {
				consume();
				tokens.push_back(Token{TokenType::open_paren });
			} else if (peek().value() == ')') {
				consume();
				tokens.push_back(Token{TokenType::close_paren });
			} else if (peek().value() == '{') {
				consume();
				tokens.push_back(Token{TokenType::open_brace });
			} else if (peek().value() == '}') {
				consume();
				tokens.push_back(Token{TokenType::close_brace });
			} else if (peek().value() == ';') {
				consume();
				tokens.push_back(Token{TokenType::semi });
			} else if (peek().value() == '+') {
				consume();
				tokens.push_back(Token{TokenType::plus });
			} else if (peek().value() == '-') {
				consume();
				tokens.push_back(Token{TokenType::minus });
			} else if (peek().value() == '*') {
				consume();
				tokens.push_back(Token{TokenType::star });
			} else if (peek().value() == '/') {
				consume();
				tokens.push_back(Token{TokenType::fslash });
			} else if (peek().value() == '=') {
				consume();
				tokens.push_back(Token{TokenType::eq });
			} else if (std::isspace(peek().value())) {
				consume();
			} else {
				std::cerr << "Unknown token " << peek().value() << std::endl;
				exit(EXIT_FAILURE);
			}
		}
		m_index = 0;
		return tokens;
	}

private:
	[[nodiscard]] inline std::optional<char> peek(int offset = 0) const {
		if (m_index + offset >= m_src.length()) {
			return {};
		} else {
			return m_src.at(m_index + offset);
		}
	}

	inline char consume() {
		return m_src.at(m_index++);
	}

	const std::string m_src;
	size_t m_index = 0;
};