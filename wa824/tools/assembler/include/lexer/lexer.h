#ifndef WA824I_LEXER_LEXER_H
#define WA824I_LEXER_LEXER_H

#include <lexer/result.h>
#include <mac.h>
#include <stdio.h>
#include <stdint.h>
#include <lexer/def.h>

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
	char lexeme[LEXER_MAX_LEXEME_LEN];
	size_t lexeme_len;
	
	union {
		
		int64_t integer;
	} val;
};

struct lexer_tok_line {
	struct lexer_tok *toks;
	size_t toks_cap;
	size_t toks_len;
};

struct lexer;

Lexer_Result
lexer_init(OUT_ struct lexer ** ALLOC_ lexer, INOUT_ FILE *in);

Lexer_Result
lexer_run(INOUT_ struct lexer *lexer);

void
lexer_deinit(TAKE_ ALLOC_ struct lexer *lexer);

/*
 * @return tok_lines_len
 * */
Lexer_Result
lexer_get_tok_lines(INOUT_ struct lexer *lexer, OUT_  const struct lexer_tok_line **tok_lines, OUT_ size_t *tok_lines_len);

#endif
