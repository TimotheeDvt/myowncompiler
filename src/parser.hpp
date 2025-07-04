#pragma once

#include <variant>
#include <cassert>

#include "./arena.hpp"
#include "tokenization.hpp"

struct NodeTermIntLit {
	Token int_lit;
};

struct NodeTermIdent {
	Token ident;
};

struct NodeExpr;

struct NodeTermParen {
	NodeExpr* expr;
};

struct NodeBinExprAdd {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

struct NodeBinExprMinus {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

struct NodeBinExprMulti {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

struct NodeBinExprDiv {
	NodeExpr* lhs;
	NodeExpr* rhs;
};

struct NodeBinExpr {
	std::variant<NodeBinExprAdd*, NodeBinExprMinus*, NodeBinExprMulti*, NodeBinExprDiv*> var;
};

struct NodeTerm {
	std::variant<NodeTermIntLit*, NodeTermIdent*, NodeTermParen*> var;
};

struct NodeExpr {
	std::variant<NodeTerm*, NodeBinExpr*> var;
};

struct NodeStmtExit {
	NodeExpr* expr;
};

struct NodeStmtLet {
	Token ident;
	NodeExpr* expr;
};

struct NodeStmtPrint {
	NodeExpr* expr;
};

struct NodeStmt;

struct NodeScope {
	std::vector<NodeStmt*> stmts;
};

struct NodeStmtIf {
	NodeExpr* cond;
	NodeScope* scope;
};

struct NodeStmtFor {
	NodeExpr* from;
	NodeExpr* to;
	NodeScope* scope;
};

struct NodeStmtAssign {
	Token ident;
	NodeExpr* expr;
};

struct NodeStmtFunction {
	Token ident;
	NodeScope* scope;
	std::vector<NodeTerm*> args;
};

struct NodeStmtFunctionCall {
	Token ident;
	std::vector<NodeExpr*> args;
};

struct NodeStmt {
	std::variant<NodeStmtExit*,
	NodeStmtLet*,
	NodeStmtPrint*,
	NodeScope*,
	NodeStmtIf*,
	NodeStmtFor*,
	NodeStmtAssign*,
	NodeStmtFunction*,
	NodeStmtFunctionCall*> var;
};

struct NodeProg {
	std::vector<NodeStmt*> stmts;
};

class Parser {
public:
	inline explicit Parser(std::vector<Token> tokens)
		: m_tokens(std::move(tokens))
		, m_allocator(1024 * 1024 * 4) // 4 mb
	{ }

	std::optional<NodeTerm*> parse_term() {
		if (auto int_lit = try_consume(TokenType::int_lit)) {
			auto term_int_lit = m_allocator.alloc<NodeTermIntLit>();
			term_int_lit->int_lit = int_lit.value();
			auto term = m_allocator.alloc<NodeTerm>();
			term->var = term_int_lit;
			return term;
		} else if (auto ident = try_consume(TokenType::ident)) {
			auto expr_ident = m_allocator.alloc<NodeTermIdent>();
			expr_ident->ident = ident.value();
			auto term = m_allocator.alloc<NodeTerm>();
			term->var = expr_ident;
			return term;
		}
		else if (auto open_paren = try_consume(TokenType::open_paren)) {
			auto expr = parse_expr();
			if (!expr.has_value()) {
				std::cerr << "Expected expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			try_consume(TokenType::close_paren, "Expected `)`");
			auto term_paren = m_allocator.alloc<NodeTermParen>();
			term_paren->expr = expr.value();
			auto term = m_allocator.alloc<NodeTerm>();
			term->var = term_paren;
			return term;
		} else {
			return {};
		}
	}

	std::optional<NodeExpr*> parse_expr(int min_prec = 0) {
		std::optional<NodeTerm*> term_lhs = parse_term();
		if (!term_lhs.has_value()) {
			return {};
		}
		auto expr_lhs = m_allocator.alloc<NodeExpr>();
		expr_lhs->var = term_lhs.value(); // 7

		while (true) {
			std::optional<Token> curr_tok = peek(); // tokentype star
			std::optional<int> prec;
			if (curr_tok.has_value()) {
				prec = bin_prec(curr_tok->type); // 1
				if (!prec.has_value() || prec.value() < min_prec) {
					break;
				}
			}
			else {
				break;
			}
			Token op = consume();
			int next_min_prec = prec.value() + 1;
			auto expr_rhs = parse_expr(next_min_prec);
			if (!expr_rhs.has_value()) {
						std::cerr << "Unable to parse expression" << std::endl;
						exit(EXIT_FAILURE);
			}
			auto expr = m_allocator.alloc<NodeBinExpr>();
			auto expr_lhs2 = m_allocator.alloc<NodeExpr>();
			if (op.type == TokenType::plus) {
				auto add = m_allocator.alloc<NodeBinExprAdd>();
				expr_lhs2->var = expr_lhs->var;
				add->lhs = expr_lhs2;
				add->rhs = expr_rhs.value();
				expr->var = add;
			}
			else if (op.type == TokenType::star) {
				auto multi = m_allocator.alloc<NodeBinExprMulti>();
				expr_lhs2->var = expr_lhs->var;
				multi->lhs = expr_lhs2;
				multi->rhs = expr_rhs.value();
				expr->var = multi;
			}
			else if (op.type == TokenType::minus) {
				auto sub = m_allocator.alloc<NodeBinExprMinus>();
				expr_lhs2->var = expr_lhs->var;
				sub->lhs = expr_lhs2;
				sub->rhs = expr_rhs.value();
				expr->var = sub;
			}
			else if (op.type == TokenType::fslash) {
				auto div = m_allocator.alloc<NodeBinExprDiv>();
				expr_lhs2->var = expr_lhs->var;
				div->lhs = expr_lhs2;
				div->rhs = expr_rhs.value();
				expr->var = div;
			}
			else {
				assert(false);
			}
			expr_lhs->var = expr;
		}
		return expr_lhs;
	}

	std::optional<NodeScope*> parse_scope() {
		if(!try_consume(TokenType::open_brace))
			return {};
		auto scope = m_allocator.alloc<NodeScope>();
		while (auto stmt = parse_stmt()) {
			scope->stmts.push_back(stmt.value());
		}
		try_consume(TokenType::close_brace, "Expected `}`");
		return scope;
	}

	std::optional<NodeStmt*> parse_stmt() {
		if (peek().value().type == TokenType::exit && peek(1).has_value()
			&& peek(1).value().type == TokenType::open_paren) {
			consume();
			consume();
			auto stmt_exit = m_allocator.alloc<NodeStmtExit>();
			if (auto node_expr = parse_expr()) {
				stmt_exit->expr = node_expr.value();
			} else {
				std::cerr << "Invalid expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			try_consume(TokenType::close_paren, "Expected `)`");
			try_consume(TokenType::semi, "Expected `;`");
			auto stmt = m_allocator.alloc<NodeStmt>();
			stmt->var = stmt_exit;
			return stmt;
		} else if (
			peek().has_value() && peek().value().type == TokenType::let && peek(1).has_value()
			&& peek(1).value().type == TokenType::ident && peek(2).has_value()
			&& peek(2).value().type == TokenType::eq) {
			consume();
			auto stmt_let = m_allocator.alloc<NodeStmtLet>();
			stmt_let->ident = consume();
			consume();
			if (auto expr = parse_expr()) {
				stmt_let->expr = expr.value();
			} else {
				std::cerr << "Invalid expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			try_consume(TokenType::semi, "Expected `;`");
			auto stmt = m_allocator.alloc<NodeStmt>();
			stmt->var = stmt_let;
			return stmt;
		} else if (peek().value().type == TokenType::print && peek(1).has_value()
			&& peek(1).value().type == TokenType::open_paren) {
			consume();
			consume();
			auto stmt_print = m_allocator.alloc<NodeStmtPrint>();
			if (auto node_expr = parse_expr()) {
				stmt_print->expr = node_expr.value();
			} else {
				std::cerr << "Invalid expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			try_consume(TokenType::close_paren, "Expected `)`");
			try_consume(TokenType::semi, "Expected `;`");
			auto stmt = m_allocator.alloc<NodeStmt>();
			stmt->var = stmt_print;
			return stmt;
		} else if (peek().value().type == TokenType::open_brace) {
			if (auto scope = parse_scope()) {
				auto stmt = m_allocator.alloc<NodeStmt>();
				stmt->var = scope.value();
				return stmt;
			} else {
				std::cerr << "Invalid scope" << std::endl;
				exit(EXIT_FAILURE);
			}
		} else if (try_consume(TokenType::_if)) {
			try_consume(TokenType::open_paren, "Expected `(`");
			auto stmt_if = m_allocator.alloc<NodeStmtIf>();
			if (auto expr = parse_expr()) {
				stmt_if->cond = expr.value();
			} else {
				std::cerr << "Invalid expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			try_consume(TokenType::close_paren, "Expected `)`");
			if (auto scope = parse_scope()) {
				stmt_if->scope = scope.value();
			} else {
				std::cerr << "Invalid scope" << std::endl;
				exit(EXIT_FAILURE);
			}
			auto stmt = m_allocator.alloc<NodeStmt>();
			stmt->var = stmt_if;
			return stmt;
		} else if (try_consume(TokenType::_for)) {
			try_consume(TokenType::open_paren, "Expected `(`");
			auto stmt_for = m_allocator.alloc<NodeStmtFor>();
			try_consume(TokenType::from, "Expected `from`");
			if (auto expr = parse_expr()) {
				stmt_for->from = expr.value();
			} else {
				std::cerr << "Invalid expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			try_consume(TokenType::to, "Expected `to`");
			if (auto expr = parse_expr()) {
				stmt_for->to = expr.value();
			} else {
				std::cerr << "Invalid expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			try_consume(TokenType::close_paren, "Expected `)`");
			if (auto scope = parse_scope()) {
				stmt_for->scope = scope.value();
			} else {
				std::cerr << "Invalid scope" << std::endl;
				exit(EXIT_FAILURE);
			}
			auto stmt = m_allocator.alloc<NodeStmt>();
			stmt->var = stmt_for;
			return stmt;
		} else if (peek().has_value() && peek().value().type == TokenType::ident &&
		peek(1).has_value() && peek(1).value().type == TokenType::eq) {
			const auto assign = m_allocator.alloc<NodeStmtAssign>();
			assign->ident = consume();
			consume();
			if (const auto expr = parse_expr()) {
				assign->expr = expr.value();
			}
			else {
					std::cerr << "Expected expression" << std::endl;
					exit(EXIT_FAILURE);
			}
			try_consume(TokenType::semi, "Expected `;`");
			auto stmt = m_allocator.emplace<NodeStmt>(assign);
			return stmt;
		} else if(peek().has_value() && peek().value().type == TokenType::ident &&
			peek(1).has_value() && peek(1).value().type == TokenType::open_paren) {
			const auto call = m_allocator.alloc<NodeStmtFunctionCall>();
			call->ident = consume();
			try_consume(TokenType::open_paren, "Expected `(`");
			while(peek().has_value() && peek().value().type != TokenType::close_paren) {
				if (auto expr = parse_expr()) {
					call->args.push_back(expr.value());
				} else {
					std::cerr << "Expected expression" << std::endl;
					exit(EXIT_FAILURE);
				}
				if (peek().has_value() && peek().value().type == TokenType::comma) {
					consume();
				}
			}
			try_consume(TokenType::close_paren, "Expected `)`");
			try_consume(TokenType::semi, "Expected `;`");
			auto stmt = m_allocator.emplace<NodeStmt>(call);
			return stmt;
		} else if (peek().has_value() && peek().value().type == TokenType::function) {
			consume();
			const auto func = m_allocator.alloc<NodeStmtFunction>();
			func->ident = consume();
			try_consume(TokenType::open_paren, "Expected `(`");
			while(peek().has_value() && peek().value().type != TokenType::close_paren) {
				if (auto ident = parse_term()) {
					func->args.push_back(ident.value());
				} else {
					std::cerr << "Expected identifier" << std::endl;
					exit(EXIT_FAILURE);
				}
				if (peek().has_value() && peek().value().type == TokenType::comma) {
					consume();
				}
			}
			try_consume(TokenType::close_paren, "Expected `)`");
			if (auto scope = parse_scope()) {
				func->scope = scope.value();
			} else {
				std::cerr << "Invalid scope" << std::endl;
			}
			auto stmt = m_allocator.emplace<NodeStmt>(func);
			return stmt;
		} else {
			return {};
		}
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
	}

private:
	[[nodiscard]] inline std::optional<Token> peek(int offset = 0) const {
		if (m_index + offset >= m_tokens.size()) {
			return {};
		} else {
			return m_tokens.at(m_index + offset);
		}
	}

	inline Token consume() {
		return m_tokens.at(m_index++);
	}

	inline Token try_consume(TokenType type, const std::string& err_msg) {
		if (peek().has_value() && peek().value().type == type) {
			return consume();
		} else {
			std::cerr << err_msg << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	inline std::optional<Token> try_consume(TokenType type)
	{
		if (peek().has_value() && peek().value().type == type) {
			return consume();
		} else {
			return {};
		}
	}

	const std::vector<Token> m_tokens;
	size_t m_index = 0;
	ArenaAllocator m_allocator;
};