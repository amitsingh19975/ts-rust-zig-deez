#if !defined(MK_PARSER_REWRITER_HPP)
#define MK_PARSER_REWRITER_HPP

#include <ast/ast_node.hpp>
#include <ast/expression.hpp>
#include <ast/decl.hpp>
#include <ast/statement.hpp>
#include <ast/types.hpp>
#include <ostream>
#include <iomanip>

namespace mk {

    static constexpr int indent_width_v = 4;

    namespace detail {

        template<typename T>
        constexpr auto initialize_value() noexcept {
            if constexpr(std::is_void_v<T>) return BlockStmt<>{};
            else return T{};
        }

        template<CtString V>
        std::ostream& rewriter_identifier_helper(std::ostream& os, IdentifierExpr<V>) {
            os << V;
            return os;
        }
        
        template<std::int64_t V>
        std::ostream& rewriter_integer_literal_helper(std::ostream& os, IntegerLiteralExpr<V>) {
            os << to_string(TypeKind::int_) << '(' << V << ')';
            return os;
        }
        
        template<bool V>
        std::ostream& rewriter_bool_literal_helper(std::ostream& os, BoolLiteralExpr<V>) {
            os << to_string(TypeKind::bool_) << '(' << (V ? "true" : "false") << ')';
            return os;
        }
        
        template<CtString V>
        std::ostream& rewriter_string_literal_helper(std::ostream& os, StringLiteralExpr<V>) {
            os << to_string(TypeKind::string) << "(\"" << V << "\")";
            return os;
        }

        template<typename T>
        std::ostream& rewriter_helper(std::ostream& os, T block, int depth, bool use_indentation) {
            auto indent = (depth + 1) * indent_width_v * static_cast<int>(use_indentation);

            if constexpr (is_block_stmt<T>::value) {
                if constexpr(T::size != 0) {
                    auto helper = []<typename...Us>(std::ostream& os, BlockStmt<Us...>, int depth, bool use_indentation) {
                        if constexpr (!(std::is_void_v<Us> || ...)) {
                            auto const indent = depth * indent_width_v * static_cast<int>(use_indentation);
                            os << std::setw(indent) << '{' << '\n';
                            rewriter_helper(os, detail::initialize_value<std::tuple<Us...>>(), depth + 1, use_indentation);
                            os << std::setw(indent) << '}';
                        }
                    };
                    helper(os, block, depth, use_indentation);
                }
            } else if constexpr (is_tuple_v<T>) {
                if constexpr (std::tuple_size_v<T> != 0) {
                    auto helper = []<typename U, typename...Us>(std::ostream& os, std::tuple<U, Us...>, int depth, bool use_indentation) {
                        if constexpr (!std::is_void_v<U>) {
                            rewriter_helper(os, detail::initialize_value<U>(), depth, use_indentation) << '\n';
                        }
                        if constexpr (sizeof... (Us) > 0) {
                            rewriter_helper(os, detail::initialize_value<std::tuple<Us...>>(), depth, use_indentation);
                        }
                    };
                    helper(os, block, depth, use_indentation);
                }
            } else if constexpr (is_if_stmt<T>::value) {
                os << std::setw(indent) << "if (";
                rewriter_helper(os, initialize_value<typename T::condition>(), depth, false) << ')' << '\n';
                rewriter_helper(os, initialize_value<typename T::then>(), depth, use_indentation) << '\n';
                if constexpr (!std::is_void_v<typename T::else_>) {
                    os << std::setw(indent) << "else";
                    rewriter_helper(os, initialize_value<typename T::else_>(), depth, use_indentation);
                }
            } else if constexpr (is_declaration_stmt<T>::value) {
                os << std::setw(indent) << "let "; 
                rewriter_helper(os, initialize_value<typename T::identifier>(), depth, use_indentation) << " = ";
                rewriter_helper(os, initialize_value<typename T::expression>(), depth, use_indentation);
                os << ';';
            } else if constexpr (is_identifier_expr<T>::value) {
                rewriter_identifier_helper(os, block);
            } else if constexpr (is_string_literal_expr<T>::value || is_bool_literal_expr<T>::value || is_integer_literal_expr<T>::value) {
                if constexpr(is_string_literal_expr<T>::value) {
                    rewriter_string_literal_helper(os, block);
                } else if constexpr(is_bool_literal_expr<T>::value) {
                    rewriter_bool_literal_helper(os, block);
                } else if constexpr(is_integer_literal_expr<T>::value) {
                    rewriter_integer_literal_helper(os, block);
                }
            } else if constexpr(is_array_literal_expr<T>::value) {
                auto helper = []<typename E, typename... Es>(std::ostream& os, ArrayLiteralExpr<E, Es...>, int depth, bool use_indentation) {
                    os << '[';
                    rewriter_helper(os, initialize_value<E>(), depth, use_indentation);
                    if constexpr (sizeof...(Es) > 0) {
                        ((os << ", ", rewriter_helper(os, initialize_value<Es>(), depth, use_indentation)), ...);
                    }
                    os << ']';
                };
                helper(os, block, depth, use_indentation);
            } else if constexpr (is_while_stmt_v<T>) {
                os << std::setw(indent) << "while (";
                rewriter_helper(os, initialize_value<typename T::condition>(), depth, false) << ')' << '\n';
                rewriter_helper(os, initialize_value<typename T::body>(), depth, use_indentation);
                os << '\n';
            } else if constexpr (is_return_stmt_v<T>) {
                os << std::setw(indent) << "return ";
                rewriter_helper(os, initialize_value<typename T::expression>(), depth, use_indentation) << ';';
            } else if constexpr (is_function_decl_v<T>) {
                os << std::setw(indent) << "fn ";
                if constexpr (!is_anon_function_decl_v<T>) {
                    rewriter_helper(os, initialize_value<typename T::id>(), depth, use_indentation);
                }
                os << '(';
                if constexpr (std::tuple_size_v<typename T::params> != 0) {
                    auto helper = []<typename U, typename...Us>(std::ostream& os, std::tuple<U, Us...>, int depth, bool use_indentation) {
                        if constexpr (!std::is_void_v<U>) {
                            rewriter_helper(os, initialize_value<U>(), depth, use_indentation);
                        }
                        if constexpr (sizeof... (Us) > 0) {
                            ((os << ", ", rewriter_helper(os, initialize_value<Us>(), depth, use_indentation)), ...);
                        }
                    };
                    helper(os, typename T::params{}, depth + 1, use_indentation);
                }
                os << ") -> ";
                rewriter_helper(os, initialize_value<typename T::return_type>(), depth, use_indentation) << ' ';

                rewriter_helper(os, initialize_value<typename T::body>(), depth, use_indentation);
            } else if constexpr (is_arg_type_v<T>) {
                if constexpr (!std::is_void_v<typename T::id>) {
                    rewriter_helper(os, initialize_value<typename T::id>(), depth, use_indentation);
                    os << ": ";
                }
                rewriter_helper(os, initialize_value<typename T::type>(), depth, use_indentation);
            } else if constexpr (is_type_v<T>) {
                if constexpr (T::kind == TypeKind::int_) {
                    os << "int";
                } else if constexpr (T::kind == TypeKind::bool_) {
                    os << "bool";
                } else if constexpr (T::kind == TypeKind::string) {
                    os << "string";
                } else if constexpr (T::kind == TypeKind::fn) {
                    os << "fn(";
                    if constexpr (std::tuple_size_v<typename T::params> != 0) {
                        auto helper = []<typename U, typename...Us>(std::ostream& os, std::tuple<U, Us...>, int depth, bool use_indentation) {
                            if constexpr (!std::is_void_v<U>) {
                                rewriter_helper(os, initialize_value<U>(), depth, use_indentation);
                            }
                            if constexpr (sizeof... (Us) > 0) {
                                ((os << ", ", rewriter_helper(os, initialize_value<Us>(), depth, use_indentation)), ...);
                            }
                        };
                        helper(os, typename T::params{}, depth + 1, use_indentation);
                    }
                    os << ") -> ";
                    rewriter_helper(os, initialize_value<typename T::return_type>(), depth, use_indentation);
                }
            } else if constexpr (is_unary_expr_v<T>) {
                os << T::op << '(';
                rewriter_helper(os, initialize_value<typename T::operand>(), depth, use_indentation);
                os << ')';
            } else if constexpr (is_binary_expr_v<T>) {
                os << T::op << '(';
                rewriter_helper(os, initialize_value<typename T::lhs>(), depth, use_indentation) << ',' << ' ';
                rewriter_helper(os, initialize_value<typename T::rhs>(), depth, use_indentation);
                os << ')';
            }
            return os;
        }

    } // namespace detail
    
    
    template<typename T>
    std::ostream& rewriter(std::ostream& os, ProgramNode<T>, bool use_indentation = false) {
        detail::rewriter_helper(os, detail::initialize_value<T>(), 0, use_indentation) << '\n';
        return os;
    }

} // namespace mk


#endif // MK_PARSER_REWRITER_HPP
