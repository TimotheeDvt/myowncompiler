#pragma once

#include "parser.hpp"
#include <cassert>
#include <map>
#include <algorithm>
#include <ranges>

class Generator {
public:
	inline explicit Generator(NodeProg prog)
		: m_prog(std::move(prog)) { }

	void gen_term(const NodeTerm* term, const bool is_function = false) {
		struct TermVisitor {
			Generator* gen;
			bool is_function;
			void operator()(const NodeTermIntLit* term_int_lit) const {
				if(is_function) {
					gen->m_functions_output << "    mov rax, " << term_int_lit->int_lit.value.value() << "\n";
				} else {
					gen->m_output << "    mov rax, " << term_int_lit->int_lit.value.value() << "\n";
				}
				gen->push("rax", is_function);
			}
			void operator()(const NodeTermIdent* term_ident) const {
				auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) { return var.name == term_ident->ident.value.value(); });
				if (it == gen->m_vars.cend()) {
					std::cerr << "Undeclared identifier: " << term_ident->ident.value.value() << std::endl;
					exit(EXIT_FAILURE);
				}
				std::stringstream offset;
				offset << "QWORD [rsp + " << (gen->m_stack_size - (*it).stack_loc - 1) * 8 << "]";
				gen->push(offset.str(), is_function);
			}
			void operator()(const NodeTermParen* term_paren) const {
				gen->gen_expr(term_paren->expr, is_function);
			}
		};
		TermVisitor visitor({ this, is_function });
		std::visit(visitor, term->var);
	}

	void genBinExpr(const NodeBinExpr* bin_expr, const bool is_function = false) {
		struct BinExprVisitor {
			Generator* gen;
			bool is_function;
			void operator()(const NodeBinExprAdd* bin_expr_add) const {
				gen->gen_expr(bin_expr_add->lhs, is_function);
				gen->gen_expr(bin_expr_add->rhs, is_function);
				gen->pop("rax", is_function);
				gen->pop("rbx", is_function);
				if(is_function) {
					gen->m_functions_output << "    add rax, rbx\n";
				} else {
					gen->m_output << "    add rax, rbx\n";
				}
				gen->push("rax", is_function);
			}
			void operator()(const NodeBinExprMinus* bin_expr_sub) const {
				gen->gen_expr(bin_expr_sub->lhs, is_function);
				gen->gen_expr(bin_expr_sub->rhs, is_function);
				gen->pop("rax", is_function);
				gen->pop("rbx", is_function);
				if(is_function) {
					gen->m_functions_output << "    sub rax, rbx\n";
				} else {
					gen->m_output << "    sub rax, rbx\n";
				}
				gen->push("rax", is_function);
			}
			void operator()(const NodeBinExprMulti* bin_expr_multi) const {
				gen->gen_expr(bin_expr_multi->lhs, is_function);
				gen->gen_expr(bin_expr_multi->rhs, is_function);
				gen->pop("rax", is_function);
				gen->pop("rbx", is_function);
				if (is_function) {
					gen->m_functions_output << "    mul rbx\n";
				} else {
					gen->m_output << "    mul rbx\n";
				}
				gen->push("rax", is_function);
			}
			void operator()(const NodeBinExprDiv* bin_expr_div) const {
				gen->gen_expr(bin_expr_div->lhs, is_function);
				gen->gen_expr(bin_expr_div->rhs, is_function);
				gen->pop("rax", is_function);
				gen->pop("rbx", is_function);
				if (is_function) {
					gen->m_functions_output << "    div rbx\n";
				} else {
					gen->m_output << "    div rbx\n";
				}
				gen->push("rax", is_function);
			}
		};

		BinExprVisitor visitor { this, is_function };
		std::visit(visitor, bin_expr->var);
	}

	void gen_expr(const NodeExpr* expr, const bool is_function = false) {
		struct ExprVisitor {
			Generator* gen;
			bool is_function;
			void operator()(const NodeTerm* term) const {
				gen->gen_term(term, is_function);
			}
			void operator()(const NodeBinExpr* bin_expr) const {
				gen->genBinExpr(bin_expr, is_function);
			}
		};

		ExprVisitor visitor { this, is_function };
		std::visit(visitor, expr->var);
	}

	void gen_stmt(const NodeStmt* stmt, const bool is_function = false) {
		struct StmtVisitor {
			Generator* gen;
			bool is_function;
			void operator()(const NodeStmtExit* stmt_exit) const {
				gen->m_is_exiting = true;
				gen->gen_expr(stmt_exit->expr, is_function);
				if(is_function) {
					gen->m_functions_output << "    mov rax, 60\n";
					gen->pop("rdi", true);
					gen->m_functions_output << "    syscall\n";
				} else {
					gen->m_output << "    mov rax, 60\n";
					gen->pop("rdi");
					gen->m_output << "    syscall\n";
				}
			}
			void operator()(const NodeStmtLet* stmt_let) const {
				auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) { return var.name == stmt_let->ident.value.value(); });
				if (it != gen->m_vars.cend()) {
					std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << std::endl;
					exit(EXIT_FAILURE);
				}
				gen->m_vars.push_back(Var { gen->m_stack_size, stmt_let->ident.value.value(), gen->gen_expr_to_str(stmt_let->expr) });
				gen->gen_expr(stmt_let->expr, is_function);
			}
			void operator()(const NodeStmtPrint* stmt_print) const {
				gen->gen_expr(stmt_print->expr, is_function);
				gen->pop("rax", is_function);

				if(is_function) {
					gen->m_functions_output << "    add rax, '0'\n";
					gen->m_functions_output << "    mov [message" << gen->m_data_counter << "], rax\n";
					gen->m_functions_output << "    mov BYTE [message" << gen->m_data_counter << " + 1], 0xA\n";
					gen->m_functions_output << "    mov rax, 1\n"; // sys_write code
					gen->m_functions_output << "    mov rdi, 1\n"; // stdout
					gen->m_functions_output << "    mov rsi, message" << gen->m_data_counter << "\n";
					gen->m_functions_output << "    mov rdx, msg_len" << gen->m_data_counter << "\n";
					gen->m_functions_output << "    syscall\n";
				} else {
					gen->m_output << "    mov rax, 1\n"; // sys_write code
					gen->m_output << "    mov rdi, 1\n"; // stdout
					gen->m_output << "    mov rsi, message" << gen->m_data_counter << "\n";
					gen->m_output << "    mov rdx, msg_len" << gen->m_data_counter << "\n";
					gen->m_output << "    syscall\n";
				}


				std::string expr_str = gen->gen_expr_to_str(stmt_print->expr);
				gen->m_data << "    message" << gen->m_data_counter << " db \"" << expr_str << "\", 0xA\n";
				gen->m_data << "    msg_len" << gen->m_data_counter << " equ $ - message" << gen->m_data_counter << "\n";
				gen->m_data_counter++;
			}
			void operator()(const NodeScope* stmt_scope) const {
				gen->begin_scope();

				for (const NodeStmt* stmt : stmt_scope->stmts) {
					gen->gen_stmt(stmt, is_function);
				}

				gen->end_scope(is_function);
			}
			void operator()(const NodeStmtIf* stmt_if) const {
				gen->gen_expr(stmt_if->cond, is_function);
				gen->pop("rax", is_function);
				if(is_function) {
					gen->m_functions_output << "    cmp rax, 0\n";
					gen->m_functions_output << "    je .if_end_" << gen->m_if_counter << "\n";
				} else {
					gen->m_output << "    cmp rax, 0\n";
					gen->m_output << "    je .if_end_" + std::to_string(gen->m_if_counter) + "\n";
				}
				for (const NodeStmt* stmt : stmt_if->scope->stmts) {
					gen->gen_stmt(stmt, is_function);
				}
				gen->create_label(".if_end_" + std::to_string(gen->m_if_counter), is_function);
				gen->m_if_counter++;
			}
			void operator()(const NodeStmtFor* stmt_for) const {
				std::string from = gen->gen_expr_to_str(stmt_for->from);
				std::string to = gen->gen_expr_to_str(stmt_for->to);
				int counter = std::stoi(to) - std::stoi(from);
				gen->m_for_counter++;
				const int local_for_counter = gen->m_for_counter;


				if(is_function) {
					gen->m_functions_output << "    mov r8, " << counter << "\n";
					gen->create_label("startloop_" + std::to_string(local_for_counter), true);

					gen->m_functions_output << "    cmp r8, 0\n";
					gen->m_functions_output << "    jz endloop_" << local_for_counter << "\n";
					gen->push("r8", true);

					gen->create_label("loop_content_" + std::to_string(local_for_counter), true);

					gen->pop("r8", true);
				} else {
					gen->m_output << "    mov r8, " << counter << "\n";
					gen->create_label("startloop_" + std::to_string(local_for_counter));

					gen->m_output << "    cmp r8, 0\n";
					gen->m_output << "    jz endloop_" << local_for_counter << "\n";
					gen->push("r8");

					gen->create_label("loop_content_" + std::to_string(local_for_counter));

					gen->pop("r8", is_function);
				}
				for (const NodeStmt* stmt : stmt_for->scope->stmts) {
					gen->gen_stmt(stmt, is_function);
				}
				gen->push("r8", is_function);

				gen->create_label("end_of_loop_content_" + std::to_string(local_for_counter), is_function);

				if(is_function) {
					gen->pop("r8", true);
					gen->m_functions_output << "    dec r8\n";
					gen->m_output << "    jmp startloop_" << local_for_counter << "\n";
				} else {
					gen->pop("r8");
					gen->m_output << "    dec r8\n";
					gen->m_output << "    jmp startloop_" << local_for_counter << "\n";
				}

				gen->create_label("endloop_" + std::to_string(local_for_counter), is_function);
			}
			void operator()(const NodeStmtAssign* stmt_assign) const {
				const auto it = std::ranges::find_if(gen->m_vars, [&](const Var& var) {
					return var.name == stmt_assign->ident.value.value();
				});
				if (it == gen->m_vars.end()) {
					std::cerr << "Undeclared identifier: " << stmt_assign->ident.value.value() << std::endl;
					exit(EXIT_FAILURE);
				}
				gen->gen_expr(stmt_assign->expr, is_function);
				gen->pop("rax", is_function);
				if(is_function) {
					gen->m_functions_output << "    mov [rsp + " << (gen->m_stack_size - it->stack_loc - 1) * 8 << "], rax\n";
				} else {
					gen->m_output << "    mov [rsp + " << (gen->m_stack_size - it->stack_loc - 1) * 8 << "], rax\n";
				}
			}
			void operator()(const NodeStmtFunction* stmt_function_declaration) const {
				const auto it = std::ranges::find_if(gen->m_functions, [&](const Func& func) {
					return func.name == stmt_function_declaration->ident.value.value();
				});
				if (it != gen->m_functions.end()) {
					std::cerr << "Already declared function identifier: " << it->name << std::endl;
					exit(EXIT_FAILURE);
				}

				gen->m_functions.push_back(Func { stmt_function_declaration->ident.value.value(), stmt_function_declaration->ident.value.value() + "_" + std::to_string(gen->m_func_counter), {} });


				gen->m_functions_output << stmt_function_declaration->ident.value.value() << "_" << gen->m_func_counter << ":\n";

				for(int i = 0; i < stmt_function_declaration->args.size(); i++) {
					gen->pop("r" + std::to_string(i + 8), true);
				}

				for (const NodeStmt* stmt : stmt_function_declaration->scope->stmts) {
					gen->gen_stmt(stmt, true);
				}

				gen->m_functions_output << "    ret\n";
				gen->m_func_counter++;
			}
			void operator()(const NodeStmtFunctionCall* stmt_function_call) const {
				const auto it = std::ranges::find_if(gen->m_functions, [&](const Func& func) {
					return func.name == stmt_function_call->ident.value.value();
				});
				if (it == gen->m_functions.end()) {
					std::cerr << "Undeclared function identifier: " << stmt_function_call->ident.value.value() << std::endl;
					exit(EXIT_FAILURE);
				}

				for(const auto & arg : stmt_function_call->args) {
					gen->push(gen->gen_expr_to_str(arg), false);
				}

				gen->m_output << "    call " << it->label << "\n";
			}
		};

		StmtVisitor visitor { this, is_function };
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
		m_output << "global _start\n";
		create_label("_start");

		for (const NodeStmt* stmt : m_prog.stmts) {
			gen_stmt(stmt, false);
		}

		if (!m_is_exiting) {
			m_output << "    mov rax, 60\n";
			m_output << "    mov rdi, 0\n";
			m_output << "    syscall\n";
		}

		if (m_func_counter > 0) {
			m_output << "\n";
			m_output << m_functions_output.str();
		}

		if (m_data.str().size() > 0) {
			m_output << "\n";
			m_output << "section .data\n";
			m_output << m_data.str();
		}

		return m_output.str();
	}

private:
	void push(const std::string& reg, const bool is_function = false) {
		if (is_function) {
			m_functions_output << "    push " << reg << "\n";
		} else {
			m_output << "    push " << reg << "\n";
		}
		m_stack_size++;
	}

	void pop(const std::string& reg, const bool is_function = false) {
		if (is_function) {
			m_functions_output << "    pop " << reg << "\n";
		} else {
			m_output << "    pop " << reg << "\n";
		}
		m_stack_size--;
	}

	void begin_scope() {
		m_scopes.push_back(m_vars.size());
	}

	void end_scope(const bool is_function = false) {
		size_t pop_count = m_vars.size() - m_scopes.back();
		if (is_function) {
			m_functions_output << "    add rsp, " << pop_count * 8 << "\n";
		} else {
			m_output << "    add rsp, " << pop_count * 8 << "\n";
		}
		m_stack_size -= pop_count;
		for (int i = 0; i < pop_count; i++) {
			m_vars.pop_back();
		}
		m_scopes.pop_back();
	}

	void create_label(const std::string& label, const bool is_function = false) {
		if (is_function) {
			m_functions_output << label << ":\n";
		} else {
			m_output << label << ":\n";
		}
	}

	struct Var {
		size_t stack_loc;
		std::string name;
		std::string value;
	};

	struct Func {
		std::string name;
		std::string label;
		std::vector<std::string> params;
	};

	const NodeProg m_prog;
	std::stringstream m_output;
	size_t m_stack_size = 0;
	std::vector<Var> m_vars {};
	size_t m_data_counter = 0;
	size_t m_for_counter = 0;
	size_t m_if_counter = 0;
	size_t m_func_counter = 0;
	bool m_is_exiting = false;
	std::stringstream m_data;
	std::vector<size_t> m_scopes {};
	std::stringstream m_functions_output;
	std::vector<Func> m_functions {};
};