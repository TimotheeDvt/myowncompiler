#pragma once

#include "parser.hpp"
#include <cassert>
#include <unordered_map>

class Generator {
public:
	inline explicit Generator(NodeProg prog)
		: m_prog(std::move(prog))
	{
	}

	void gen_term(const NodeTerm* term) {
		struct TermVisitor {
			Generator* gen;
			void operator()(const NodeTermIntLit* term_int_lit) const {
				gen->m_output << "    mov rax, " << term_int_lit->int_lit.value.value() << "\n";
				gen->push("rax");
			}
			void operator()(const NodeTermIdent* term_ident) const {
				if (gen->m_vars.find(term_ident->ident.value.value()) == gen->m_vars.end()) {
					std::cerr << "Undeclared identifier: " << term_ident->ident.value.value() << std::endl;
					exit(EXIT_FAILURE);
				}
				const auto& var = gen->m_vars.at(term_ident->ident.value.value());
				std::stringstream offset;
				offset << "QWORD [rsp + " << (gen->m_stack_size - var.stack_loc - 1) * 8 << "]\n";
				gen->push(offset.str());
			}
		};
		TermVisitor visitor({ this});
		std::visit(visitor, term->var);
	}

	void gen_expr(const NodeExpr* expr)
	{
		struct ExprVisitor {
			Generator* gen;
			void operator()(const NodeTerm* term) const
			{
				gen->gen_term(term);
			}
			void operator()(const NodeBinExpr* bin_expr) const
			{
				gen->gen_expr(bin_expr->add->lhs);
				gen->gen_expr(bin_expr->add->rhs);
				gen->pop("rax");
				gen->pop("rbx");
				gen->m_output << "    add rax, rbx\n";
				gen->push("rax");
			}
		};

		ExprVisitor visitor { this };
		std::visit(visitor, expr->var);
	}

	void gen_stmt(const NodeStmt* stmt)
	{
		struct StmtVisitor {
			Generator* gen;
			void operator()(const NodeStmtExit* stmt_exit) const
			{
				gen->m_is_exiting = true;
				gen->gen_expr(stmt_exit->expr);
				gen->m_output << "    mov rax, 60\n";
				gen->pop("rdi");
				gen->m_output << "    syscall\n";
			}
			void operator()(const NodeStmtLet* stmt_let) const
			{
				if (gen->m_vars.find(stmt_let->ident.value.value()) != gen->m_vars.end()) {
					std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << std::endl;
					exit(EXIT_FAILURE);
				}
				gen->m_vars.insert({ stmt_let->ident.value.value(), Var { gen->m_stack_size, gen->gen_expr_to_str(stmt_let->expr) } });
				gen->gen_expr(stmt_let->expr);
			}
			void operator()(const NodeStmtPrint* stmt_print) const
			{
				gen->m_output << "    mov rax, 1\n"; // sys_write code
				gen->m_output << "    mov rdi, 1\n"; // stdout
				gen->m_output << "    mov rsi, message\n";
				gen->m_output << "    mov rdx, msg_len\n";
				gen->m_output << "    syscall\n";


				std::string expr_str = gen->gen_expr_to_str(stmt_print->expr);
				gen->m_data << "    message db \"" << expr_str << "\", 0xA\n";
				gen->m_data << "    msg_len equ $ - message\n";
			}
		};

		StmtVisitor visitor { this };
		std::visit(visitor, stmt->var);
	}

	// New function to convert an expression to a string
	std::string gen_expr_to_str(const NodeExpr* expr) {
		std::stringstream ss;
		struct ExprVisitor {
			Generator* gen;
			std::stringstream& ss;
			void operator()(const NodeTerm* term) const
			{
				ss << gen->gen_term_to_str(term);
			}
			void operator()(const NodeBinExpr* bin_expr) const
			{
				ss << gen->gen_expr_to_str(bin_expr->add->lhs);
				ss << " + ";
				ss << gen->gen_expr_to_str(bin_expr->add->rhs);
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
			void operator()(const NodeTermIntLit* term_int_lit) const
			{
				ss << term_int_lit->int_lit.value.value();
			}
			void operator()(const NodeTermIdent* term_ident) const
			{
				if (gen->m_vars.find(term_ident->ident.value.value()) != gen->m_vars.end()) {
					const auto& var = gen->m_vars.at(term_ident->ident.value.value());
					ss << var.value.value();
				} else {
					ss << term_ident->ident.value.value();
				}
			}
		};
		TermVisitor visitor { this, ss };
		std::visit(visitor, term->var);
		return ss.str();
	}

	[[nodiscard]] std::string gen_prog()
	{
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
	void push(const std::string& reg)
	{
		m_output << "    push " << reg << "\n";
		m_stack_size++;
	}

	void pop(const std::string& reg)
	{
		m_output << "    pop " << reg << "\n";
		m_stack_size--;
	}

	struct Var {
		size_t stack_loc;
		std::optional<std::string> value;
	};

	const NodeProg m_prog;
	std::stringstream m_output;
	size_t m_stack_size = 0;
	std::unordered_map<std::string, Var> m_vars {};
	bool m_is_exiting = false;
	std::stringstream m_data;
};