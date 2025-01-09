#pragma once

#include "parser.hpp"
#include <cassert>
#include <map>
#include <algorithm>

class Generator {
public:
	inline explicit Generator(NodeProg prog)
		: m_prog(std::move(prog)) { }

	void gen_term(const NodeTerm* term) {
		struct TermVisitor {
			Generator* gen;
			void operator()(const NodeTermIntLit* term_int_lit) const {
				gen->m_output << "    mov rax, " << term_int_lit->int_lit.value.value() << "\n";
				gen->push("rax");
			}
			void operator()(const NodeTermIdent* term_ident) const {
				auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) { return var.name == term_ident->ident.value.value(); });
				if (it == gen->m_vars.cend()) {
					std::cerr << "Undeclared identifier: " << term_ident->ident.value.value() << std::endl;
					exit(EXIT_FAILURE);
				}
				std::stringstream offset;
				offset << "QWORD [rsp + " << (gen->m_stack_size - (*it).stack_loc - 1) * 8 << "]";
				gen->push(offset.str());
			}
			void operator()(const NodeTermParen* term_paren) const {
				gen->gen_expr(term_paren->expr);
			}
		};
		TermVisitor visitor({ this});
		std::visit(visitor, term->var);
	}

	void genBinExpr(const NodeBinExpr* bin_expr) {
		struct BinExprVisitor {
			Generator* gen;
			void operator()(const NodeBinExprAdd* bin_expr_add) const {
				gen->gen_expr(bin_expr_add->lhs);
				gen->gen_expr(bin_expr_add->rhs);
				gen->pop("rax");
				gen->pop("rbx");
				gen->m_output << "    add rax, rbx\n";
				gen->push("rax");
			}
			void operator()(const NodeBinExprMinus* bin_expr_sub) const {
				gen->gen_expr(bin_expr_sub->lhs);
				gen->gen_expr(bin_expr_sub->rhs);
				gen->pop("rax");
				gen->pop("rbx");
				gen->m_output << "    sub rax, rbx\n";
				gen->push("rax");
			}
			void operator()(const NodeBinExprMulti* bin_expr_multi) const {
				gen->gen_expr(bin_expr_multi->lhs);
				gen->gen_expr(bin_expr_multi->rhs);
				gen->pop("rax");
				gen->pop("rbx");
				gen->m_output << "    mul rbx\n";
				gen->push("rax");
			}
			void operator()(const NodeBinExprDiv* bin_expr_div) const {
				gen->gen_expr(bin_expr_div->lhs);
				gen->gen_expr(bin_expr_div->rhs);
				gen->pop("rax");
				gen->pop("rbx");
				gen->m_output << "    div rbx\n";
				gen->push("rax");
			}
		};

		BinExprVisitor visitor { this };
		std::visit(visitor, bin_expr->var);
	}

	void gen_expr(const NodeExpr* expr) {
		struct ExprVisitor {
			Generator* gen;
			void operator()(const NodeTerm* term) const {
				gen->gen_term(term);
			}
			void operator()(const NodeBinExpr* bin_expr) const {
				gen->genBinExpr(bin_expr);
			}
		};

		ExprVisitor visitor { this };
		std::visit(visitor, expr->var);
	}

	void gen_stmt(const NodeStmt* stmt) {
		struct StmtVisitor {
			Generator* gen;
			void operator()(const NodeStmtExit* stmt_exit) const {
				gen->m_is_exiting = true;
				gen->gen_expr(stmt_exit->expr);
				gen->m_output << "    mov rax, 60\n";
				gen->pop("rdi");
				gen->m_output << "    syscall\n";
			}
			void operator()(const NodeStmtLet* stmt_let) const {
				auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) { return var.name == stmt_let->ident.value.value(); });
				if (it != gen->m_vars.cend()) {
					std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << std::endl;
					exit(EXIT_FAILURE);
				}
				gen->m_vars.push_back(Var { gen->m_stack_size, stmt_let->ident.value.value(), gen->gen_expr_to_str(stmt_let->expr) });
				gen->gen_expr(stmt_let->expr);
			}
			void operator()(const NodeStmtPrint* stmt_print) const {
				gen->m_output << "    mov rax, 1\n"; // sys_write code
				gen->m_output << "    mov rdi, 1\n"; // stdout
				gen->m_output << "    mov rsi, message" << gen->m_data_counter << "\n";
				gen->m_output << "    mov rdx, msg_len" << gen->m_data_counter << "\n";
				gen->m_output << "    syscall\n";


				std::string expr_str = gen->gen_expr_to_str(stmt_print->expr);
				gen->m_data << "    message" << gen->m_data_counter << " db \"" << expr_str << "\", 0xA\n";
				gen->m_data << "    msg_len" << gen->m_data_counter << " equ $ - message" << gen->m_data_counter << "\n";
				gen->m_data_counter++;
			}
			void operator()(const NodeStmtScope* stmt_scope) const {
				gen->begin_scope();

				for (const NodeStmt* stmt : stmt_scope->stmts) {
					gen->gen_stmt(stmt);
				}

				gen->end_scope();
			}
		};

		StmtVisitor visitor { this };
		std::visit(visitor, stmt->var);
	}

	std::string gen_bin_expr_to_str(const NodeBinExpr* expr) {
		std::stringstream ss;
		struct BinExprVisitor {
			Generator* gen;
			std::stringstream& ss;
			void operator()(const NodeBinExprAdd* bin_expr_add) const {
				ss << std::to_string(std::stoi(gen->gen_expr_to_str(bin_expr_add->lhs)) + std::stoi(gen->gen_expr_to_str(bin_expr_add->rhs)));
			}
			void operator()(const NodeBinExprMinus* bin_expr_sub) const {
				ss << std::to_string(std::stoi(gen->gen_expr_to_str(bin_expr_sub->lhs)) - std::stoi(gen->gen_expr_to_str(bin_expr_sub->rhs)));
			}
			void operator()(const NodeBinExprMulti* bin_expr_multi) const {
				ss << std::to_string(std::stoi(gen->gen_expr_to_str(bin_expr_multi->lhs)) * std::stoi(gen->gen_expr_to_str(bin_expr_multi->rhs)));
			}
			void operator()(const NodeBinExprDiv* bin_expr_div) const {
				ss << std::to_string(std::stoi(gen->gen_expr_to_str(bin_expr_div->lhs)) / std::stoi(gen->gen_expr_to_str(bin_expr_div->rhs)));
			}
		};

		BinExprVisitor visitor { this, ss };
		std::visit(visitor, expr->var);
		return ss.str();
	}

	std::string gen_expr_to_str(const NodeExpr* expr) {
		std::stringstream ss;
		struct ExprVisitor {
			Generator* gen;
			std::stringstream& ss;
			void operator()(const NodeTerm* term) const {
				ss << gen->gen_term_to_str(term);
			}
			void operator()(const NodeBinExpr* bin_expr) const {
				ss << gen->gen_bin_expr_to_str(bin_expr);
			}
		};
		ExprVisitor visitor { this, ss };
		std::visit(visitor, expr->var);
		return ss.str();
	}

	std::string gen_term_to_str(const NodeTerm* term) {
		std::stringstream ss;
		struct TermVisitor {
			Generator* gen;
			std::stringstream& ss;
			void operator()(const NodeTermIntLit* term_int_lit) const {
				ss << term_int_lit->int_lit.value.value();
			}
			void operator()(const NodeTermIdent* term_ident) const {
				auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) {
						return var.name == term_ident->ident.value.value();
				});
				if (it != gen->m_vars.cend()) {
					ss << (*it).value;
				} else {
					ss << term_ident->ident.value.value();
				}
			}
			void operator()(const NodeTermParen* term_paren) const {
				ss << gen->gen_expr_to_str(term_paren->expr);
			}
		};
		TermVisitor visitor { this, ss };
		std::visit(visitor, term->var);
		return ss.str();
	}

	[[nodiscard]] std::string gen_prog() {
		m_output << "global _start\n_start:\n";

		for (const NodeStmt* stmt : m_prog.stmts) {
			gen_stmt(stmt);
		}

		if (!m_is_exiting) {
			m_output << "    mov rax, 60\n";
			m_output << "    mov rdi, 0\n";
			m_output << "    syscall\n";
		}

		if (m_data.str().size() > 0) {
			m_output << "\n";
			m_output << "section .data\n";
			m_output << m_data.str();
		}

		return m_output.str();
	}

private:
	void push(const std::string& reg) {
		m_output << "    push " << reg << "\n";
		m_stack_size++;
	}

	void pop(const std::string& reg) {
		m_output << "    pop " << reg << "\n";
		m_stack_size--;
	}

	void begin_scope() {
		m_scopes.push_back(m_vars.size());
	}

	void end_scope() {
		size_t pop_count = m_vars.size() - m_scopes.back();
		m_output << "    add rsp, " << pop_count * 8 << "\n";
		m_stack_size -= pop_count;
		for (int i = 0; i < pop_count; i++) {
			m_vars.pop_back();
		}
		m_scopes.pop_back();
	}

	struct Var {
		size_t stack_loc;
		std::string name;
		std::string value;
	};

	const NodeProg m_prog;
	std::stringstream m_output;
	size_t m_stack_size = 0;
	std::vector<Var> m_vars {};
	size_t m_data_counter = 0;
	bool m_is_exiting = false;
	std::stringstream m_data;
	std::vector<size_t> m_scopes {};
};