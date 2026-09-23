#ifndef WA824I_LEXER_LEXER_H_
#define WA824I_LEXER_LEXER_H_

#include <as/err.h>
#include <lexer/def.h>
#include <lexer/result.h>
#include <mac.h>
#include <stdint.h>
#include <stdio.h>
#include <tok/tok.h>

struct lexer {
	FILE *in;
	struct lexer_tok_line *tok_lines;
	size_t tok_lines_cap;
	size_t tok_lines_len;
	struct as_diag *diag;
};

void lexer_init(OUT_ struct lexer *lexer, INOUT_ FILE *in,
		OUT_ struct as_diag *diag);

Lexer_Result lexer_run(INOUT_ struct lexer *lexer);

void lexer_deinit(INOUT_ struct lexer *lexer);

size_t lexer_get_tok_lines(INOUT_ struct lexer *lexer,
			   OUT_ const struct lexer_tok_line **tok_lines);

#endif
