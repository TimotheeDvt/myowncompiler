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
        int line_count = 1;
        while (peek().has_value()) {
            if (std::isalpha(peek().value())) {
                buf.push_back(consume());
                while (peek().has_value() && std::isalnum(peek().value())) {
                    buf.push_back(consume());
                }
                if (buf == "exit") {
                    tokens.push_back({ TokenType::exit });
                    buf.clear();
                }
                else if (buf == "let") {
                    tokens.push_back({ TokenType::let });
                    buf.clear();
                }
                else {
                    tokens.push_back({ TokenType::ident, buf });
                    buf.clear();
                }
            }
            else if (std::isdigit(peek().value())) {
                buf.push_back(consume());
                while (peek().has_value() && std::isdigit(peek().value())) {
                    buf.push_back(consume());
                }
                tokens.push_back({ TokenType::int_lit, buf });
                buf.clear();
            }
            else if (peek().value() == '(') {
                consume();
                tokens.push_back({ TokenType::open_paren });
            }
            else if (peek().value() == ')') {
                consume();
                tokens.push_back({ TokenType::close_paren });
            }
            else if (peek().value() == ';') {
                consume();
                tokens.push_back({ TokenType::semi });
            }
            else if (peek().value() == '=') {
                consume();
                tokens.push_back({ TokenType::eq });
            }
            else if (peek().value() == '\n') {
                consume();
                line_count++;
            }
            else if (std::isspace(peek().value())) {
                consume();
            }
            else {
                std::cerr << "Invalid token " << peek().has_value() << std::endl;
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
		}
		return m_src.at(m_index + offset);
	}

	inline char consume() {
		return m_src.at(m_index++);
	}

	const std::string& m_src;
	int m_index = 0;
};