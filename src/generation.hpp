#pragma once
#include <sstream>

#include "parser.hpp"
#include <unordered_map>

class Generator {
public:
	inline Generator(NodeProg prog):
		m_prog(std::move(prog))
	{};
	
	void gen_expr(const NodeExpr& expr) {
		struct ExprVisitor {
			Generator* gen;
			void operator()(const NodeExprIntLit& expr_int_lit) {
				gen->m_output << "    mov rax, " << expr_int_lit.int_lit.value.value() << "\n";
				gen->push("rax");
			}
			void operator()(const NodeExprIdent& expr_ident) {
				if (gen->m_vars.find(expr_ident.ident.value.value()) != gen->m_vars.end()) {
					std::cerr << "Undeclared identifier " << expr_ident.ident.value.value() << std::endl;
					exit(EXIT_FAILURE);
				}
				const auto& var = gen->m_vars.at(expr_ident.ident.value.value());
				std::stringstream offset;
				offset << "[rsp + " << (gen->m_stack_size - var.stack_loc) << "]";
				gen->push(offset.str());
			}
		};

		ExprVisitor visitor{this};
		std::visit(visitor, expr.var);
	}


	void gen_stmt(const NodeStmt& stmt) {
		std::stringstream output;
		struct StmtVisitor {
		Generator* gen;
		void operator()(const NodeStmtExit& stmt_exit) const {
			gen->gen_expr(stmt_exit.expr);
			gen->m_output << "    mov rax, 60\n";
			gen->pop("rdi");
			gen->m_output << "    syscall\n";
			gen->m_is_exit = true;
		}
		void operator()(const NodeStmtLet& stmt_let) const {
			if(gen->m_vars.find(stmt_let.ident.ident.value.value()) != gen->m_vars.end()) {
				std::cerr << "Variable already exists" << std::endl;
				exit(EXIT_FAILURE);
			}
			gen->m_vars.insert({stmt_let.ident.ident.value.value(), {gen->m_stack_size}});
			gen->gen_expr(stmt_let.expr);
		}
		void operator()(const NodeStmtPrint& stmt_print) const {
			std::cout << "NOT IMPLEMENTED : PRINT" << std::endl;
		}
	};

		StmtVisitor visitor{this};
		std::visit(visitor, stmt.var);
	};

	[[nodiscard]] inline std::string gen_prog() {
		std::stringstream output;		
		m_output << "global _start\n_start:\n";

		for (const NodeStmt& stmt : m_prog.stmts) {
			gen_stmt(stmt);
		}

		if (!m_is_exit) {
			m_output << "    mov rax, 60\n";
			m_output << "    mov rdi, 0\n";
			m_output << "    syscall";
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

	struct Var {
		size_t stack_loc;
	};

	const NodeProg m_prog;
	std::stringstream m_output;
	size_t m_stack_size = 0;
	std::unordered_map<std::string, Var> m_vars {};
	bool m_is_exit = false;
};

// [[nodiscard]] inline std::string generatePrint() {
// 	std::stringstream output;
// 	m_output << "global _start\n_start:\n";
// 	m_output << "    mov rax, 1\n";
// 	m_output << "    mov rdi, 1\n";
// 	m_output << "    mov rsi, message\n";
// 	m_output << "    mov rdx, msg_len\n";
// 	m_output << "    syscall\n";

// 	m_output << "    mov rax, 60\n";
// 	m_output << "    mov rdi, 0\n";
// 	m_output << "    syscall\n";

// 	m_output << "section .data\n";
// 	m_output << "    message db \'" << m_prog.expr.int_lit.value.value() << "\', 0xA\n";
// 	m_output << "    msg_len equ $ - message";

// 	return output.str();
// }