#pragma once

#include "tokenization.hpp"
#include <variant>

struct NodeExprIntLit {
	Token int_lit;
};
struct NodeExprIdent {
	Token ident;
};
struct NodeExpr {
	std::variant<NodeExprIntLit, NodeExprIdent> var;
};
struct NodeStmtExit {
	NodeExpr expr;
};
struct NodeStmtPrint {
	NodeExpr expr;
};
struct NodeStmtLet {
	NodeExprIdent ident;
	NodeExpr expr;
};
struct NodeStmt {
	std::variant<NodeStmtExit, NodeStmtPrint, NodeStmtLet> var;
};
struct NodeProg {
	std::vector<NodeStmt> stmts;
};


class Parser {
public:
	inline explicit Parser(std::vector<Token> tokens):
		m_tokens(std::move(tokens))
	{};

	std::optional<NodeExpr> parse_expr() {
		if (peek().has_value() && peek().value().type == TokenType::int_lit) {
			return NodeExpr{NodeExprIntLit{consume()}};
		} else if (peek().has_value() && peek().value().type == TokenType::ident) {
			return NodeExpr{NodeExprIdent{consume()}};
		}
		return {};
	}

	std::optional<NodeStmt> parse_stmt() {
		if (peek().has_value() && peek().value().type == TokenType::exit && peek(1).has_value() && peek(1).value().type == TokenType::open_paren) {
			consume();
			consume();
			NodeStmtExit stmt_exit;
			if(auto expr_node = parse_expr()) {
				stmt_exit = NodeStmtExit{expr_node.value()};
			} else {
				std::cerr << "Invalid expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			if (peek().has_value() && peek().value().type == TokenType::close_paren) {
				consume();
			} else {
				std::cerr << "Expected `)`" << std::endl;
				exit(EXIT_FAILURE);
			}
			if (peek().has_value() && peek().value().type == TokenType::semi) {
				consume();
			} else {
				std::cerr << "Expected ; after int literal" << std::endl;
				exit(EXIT_FAILURE);
			}
			return NodeStmt{stmt_exit};
		} else if (peek().has_value() && peek().value().type == TokenType::print && peek(1).has_value() && peek(1).value().type == TokenType::open_paren) {
			consume();
			consume();
			NodeStmtPrint stmt_print;
			if(auto expr_node = parse_expr()) {
				stmt_print = NodeStmtPrint{expr_node.value()};
			} else {
				std::cerr << "Invalid expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			if (peek().has_value() && peek().value().type == TokenType::close_paren) {
				consume();
			} else {
				std::cerr << "Expected `)`" << std::endl;
				exit(EXIT_FAILURE);
			}
			if (peek().has_value() && peek().value().type == TokenType::semi) {
				consume();
			} else {
				std::cerr << "Expected ; after int literal" << std::endl;
				exit(EXIT_FAILURE);
			}
			return NodeStmt{stmt_print};
		} else if (
			peek().has_value() && peek().value().type == TokenType::let &&
			peek(1).has_value() && peek(1).value().type == TokenType::ident &&
			peek(2).has_value() && peek(2).value().type == TokenType::eq
		) {
			consume();
			auto stmt_let = NodeStmtLet{consume()};
			consume();
			if (auto expr_node = parse_expr()) {
				stmt_let.expr = expr_node.value();
			} else {
				std::cerr << "Invalid expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			if (peek().has_value() && peek().value().type == TokenType::semi) {
				consume();
			} else {
				std::cerr << "Expected ; after int literal" << std::endl;
				exit(EXIT_FAILURE);
			}
			return NodeStmt{stmt_let};
		}
		return {};
	}

	std::optional<NodeProg> parse_prog() {
		NodeProg prog;
		while (peek().has_value()) {
			if (auto stmt = parse_stmt()) {
				prog.stmts.push_back(stmt.value());
			} else {
				std::cerr << "Invalid statement" << std::endl;
				exit(EXIT_FAILURE);
			}
		}
		return prog;
	};
private:

	[[nodiscard]] inline std::optional<Token> peek(int offset = 0) const {
		if (m_index + offset >= m_tokens.size()) {
			return {};
		}
		return m_tokens.at(m_index + offset);
	}

	inline Token consume() {
		return m_tokens.at(m_index++);
	}

	std::vector<Token> m_tokens;
	size_t m_index = 0;
};