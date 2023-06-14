#include <catch2/catch_test_macros.hpp>
#include <lexer/lexer.hpp>
#include "lexer_test_utils.hpp"

template<std::size_t M, std::size_t N>
constexpr std::array<mk::Token, M> lex_source(mk::CtString<N> source, bool skip_whitespace = true) noexcept {
    auto temp = mk::Lexer(source);
    std::size_t curr_ptr{};
    std::array<mk::Token, M> tokens;

    while (curr_ptr < M) {
        mk::Token token = temp.next();
        if (token.is(mk::TokenKind::unknown)) break;
        if (skip_whitespace && token.is_one_of(mk::TokenKind::whitespace, mk::TokenKind::newline)) continue;
        tokens[curr_ptr++] = token;

        if (token.is_one_of(mk::TokenKind::eof, mk::TokenKind::illegal)) break;
    }
    return tokens;
}

template<std::size_t N, std::size_t M>
constexpr bool assert_eq_tokens(std::array<mk::Token, M> const& tokens, std::array<mk::Token, N> const& expected) noexcept {
    static_assert(M >= N, "Expected array is larger than actual array");
    return std::equal(expected.begin(), expected.end(), tokens.begin(), [](mk::Token const& l, mk::Token const& r) {
        return l.is(r.kind());
    });
}

TEST_CASE("validating tokens", "[constexpr][lexer][tokens]") {
    
    constexpr auto expected_tokens = mk::test::TestInput1::tokens;
    constexpr auto source = mk::test::TestInput1::source;
    constexpr auto lexer_tokens = lex_source<1000>(source);
    static_assert(assert_eq_tokens(lexer_tokens, expected_tokens));
}

TEST_CASE("validating comments", "[constexpr][lexer][comments]") {
    SECTION("single line comment") {
        constexpr auto source = mk::CtString("// This is a comment");
        constexpr auto lexer_tokens = lex_source<2>(source);
        static_assert(lexer_tokens[0].is_comment());
        static_assert(lexer_tokens[1].is_eof());
    }

    SECTION("multiline comment") {
        constexpr auto source = mk::CtString(R"(
            /*
                This is a multiline comment
            */
        )");
        constexpr auto lexer_tokens = lex_source<2>(source);
        static_assert(lexer_tokens[0].is_comment());
        static_assert(lexer_tokens[1].is_eof());
    }
}

TEST_CASE("validating string", "[constexpr][lexer][string]") {
    SECTION("simple string") {
        constexpr auto source = mk::CtString(R"("Hello, World!")");
        constexpr auto lexer_tokens = lex_source<2>(source);
        static_assert(lexer_tokens[0].is_string_literal());
        static_assert(lexer_tokens[0].lexeme() == R"(Hello, World!)");
        static_assert(lexer_tokens[1].is_eof());
    }

    SECTION("string with escaped quotes") {
        constexpr auto source = mk::CtString(R"("Hello, \"World!\"")");
        constexpr auto lexer_tokens = lex_source<2>(source);
        static_assert(lexer_tokens[0].is_string_literal());
        static_assert(lexer_tokens[0].lexeme() == R"(Hello, \"World!\")");
        static_assert(lexer_tokens[1].is_eof());
    }

    SECTION("string with escaped backslash") {
        constexpr auto source = mk::CtString(R"("Hello, \\World!\"")");
        constexpr auto lexer_tokens = lex_source<2>(source);
        static_assert(lexer_tokens[0].is_string_literal());
        static_assert(lexer_tokens[0].lexeme() == R"(Hello, \\World!\")");
        static_assert(lexer_tokens[1].is_eof());
    }
}

