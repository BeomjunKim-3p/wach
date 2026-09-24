#ifndef WA824I_TOK_TOK_H_
#define WA824I_TOK_TOK_H_

#include "def.h"
#include <stddef.h>
#include <stdint.h>

enum LEXER_TOK_KIND_ {
	/*
	 * For initialize
	 * */
	LEXER_TOK_KIND_INIT,
	LEXER_TOK_KIND_NEWLINE,
	LEXER_TOK_KIND_EOF,
	LEXER_TOK_KIND_IDENTIFIER,
	LEXER_TOK_KIND_STRING,
	LEXER_TOK_KIND_COMMA,
	LEXER_TOK_KIND_INTEGER,
	LEXER_TOK_KIND_COLON,
	LEXER_TOK_KIND_L_PARENTHESES,
	LEXER_TOK_KIND_R_PARENTHESES,
};

struct lexer_tok {
	enum LEXER_TOK_KIND_ kind;
	char lexeme[TOK_MAX_LEXEME_LEN + 1];
	size_t lexeme_len;

	/* Start at */
	size_t line;
	size_t column;

	union {
		int64_t integer;
	} val;
};

struct lexer_tok_line {
	struct lexer_tok *toks;
	size_t toks_cap;
	size_t toks_len;
};

#endif