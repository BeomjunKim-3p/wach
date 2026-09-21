#include <assert.h>
#include <ctype.h>
#include <lexer/lexer.h>
#include <lexer/result.h>
#include <pp/def.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_TOK_LINES_LEN 50
#define DEFAULT_TOKS_LEN 6

#define IS_ASCII(ch_) ((bool)((unsigned char)(ch_) <= 0x7Fu))

struct lexer {
	FILE *in;
	struct lexer_tok_line *tok_lines;
	size_t tok_lines_cap;
	size_t tok_lines_len;
};

enum LEXER_STATE_ {
	LEXER_STATE_NORMAL,
	LEXER_STATE_IDENTIFIER,
	LEXER_STATE_STRING,
	LEXER_STATE_ASCII,
	LEXER_STATE_INTEGER,
};

static void
s_deinit_tok(struct lexer_tok *tok)
{
	if (!tok)
		return;

	tok->kind = LEXER_TOK_KIND_INIT;
	tok->lexeme_len = 0;
	tok->val.integer = 0;
	memset(tok->lexeme, 0, LEXER_MAX_LEXEME_LEN);
}

static void
s_deinit_tok_line(struct lexer_tok_line *tok_line)
{
	if (!tok_line)
		return;

	tok_line->toks_len = 0;

	if (!tok_line->toks) {
		tok_line->toks_cap = 0;
		return;
	}

	for (size_t i = 0; i < tok_line->toks_cap; ++i) {
		s_deinit_tok(&tok_line->toks[i]);
	}

	free(tok_line->toks);

	tok_line->toks_cap = 0;
}

void
lexer_deinit(struct lexer *lexer)
{
	if (!lexer)
		return;

	lexer->tok_lines_len = 0;

	if (!lexer->tok_lines) {
		lexer->tok_lines_cap = 0;
		return;
	}

	for (size_t i = 0; i < lexer->tok_lines_cap; ++i) {
		s_deinit_tok_line(&lexer->tok_lines[i]);
	}

	free(lexer->tok_lines);

	lexer->tok_lines_cap = 0;
}

static void
s_init_tok(struct lexer_tok *tok)
{
	assert(tok);

	tok->kind = LEXER_TOK_KIND_INIT;
	tok->lexeme_len = 0;
	tok->val.integer = 0;
	memset(tok->lexeme, 0, LEXER_MAX_LEXEME_LEN);
}

static Lexer_Result
s_resize_toks(struct lexer_tok_line *tok_line)
{
	assert(tok_line);

	struct lexer_tok *tmp_ptr = NULL;

	if (!tok_line->toks) {
		tmp_ptr = (struct lexer_tok *)malloc(sizeof(struct lexer_tok) *
						     DEFAULT_TOKS_LEN);
	} else {
		tmp_ptr = (struct lexer_tok *)realloc(
			tok_line->toks,
			sizeof(struct lexer_tok) *
				(tok_line->toks_cap + DEFAULT_TOKS_LEN));
	}

	if (!tmp_ptr)
		return LEXER_RESULT_ERR(LEXER_ERR_OOM);

	tok_line->toks = tmp_ptr;

	for (size_t i = tok_line->toks_cap;
	     i < tok_line->toks_cap + DEFAULT_TOKS_LEN; ++i) {
		s_init_tok(&tok_line->toks[i]);
	}

	tok_line->toks_cap += DEFAULT_TOKS_LEN;

	return LEXER_RESULT_OK;
}
static Lexer_Result
s_init_tok_line(struct lexer_tok_line *tok_line)
{
	assert(tok_line);

	tok_line->toks = (struct lexer_tok *)malloc(sizeof(struct lexer_tok) *
						    DEFAULT_TOKS_LEN);
	if (!tok_line->toks)
		return LEXER_RESULT_ERR(LEXER_ERR_INTERNAL);

	tok_line->toks_cap = DEFAULT_TOKS_LEN;
	tok_line->toks_len = 0;

	for (size_t i = 0; i < DEFAULT_TOKS_LEN; ++i)
		s_init_tok(&tok_line->toks[i]);

	return LEXER_RESULT_OK;
}

static Lexer_Result
s_resize_tok_lines(struct lexer *lexer)
{
	assert(lexer);

	struct lexer_tok_line *tmp_ptr = NULL;

	if (!lexer->tok_lines) {
		tmp_ptr = (struct lexer_tok_line *)malloc(
			sizeof(struct lexer_tok_line) * DEFAULT_TOK_LINES_LEN);
	} else {
		tmp_ptr = (struct lexer_tok_line *)realloc(
			lexer->tok_lines,
			sizeof(struct lexer_tok_line) *
				(lexer->tok_lines_cap + DEFAULT_TOK_LINES_LEN));
	}

	if (!tmp_ptr)
		return LEXER_RESULT_ERR(LEXER_ERR_INTERNAL);

	lexer->tok_lines = tmp_ptr;

	const size_t initial_cap = lexer->tok_lines_cap;
	for (size_t i = 0; i < DEFAULT_TOK_LINES_LEN; ++i) {
		const Lexer_Result result =
			s_init_tok_line(&lexer->tok_lines[initial_cap + i]);
		LEXER_RET_IF_ERR(result);

		lexer->tok_lines_cap++;
	}

	return LEXER_RESULT_OK;
}

Lexer_Result
lexer_get_tok_lines(struct lexer *lexer,
		    const struct lexer_tok_line **tok_lines,
		    size_t *tok_lines_len)
{
	if (!lexer || !tok_lines || !tok_lines_len)
		return LEXER_RESULT_ERR(LEXER_ERR_INVAL_PARAM);

	if (!lexer->tok_lines)
		return LEXER_RESULT_ERR(LEXER_ERR_INVAL_PARAM);

	*tok_lines = lexer->tok_lines;
	*tok_lines_len = lexer->tok_lines_len;

	return LEXER_RESULT_OK;
}

Lexer_Result
lexer_init(struct lexer **lexer, FILE *in)
{
	if (!lexer || !in)
		return LEXER_RESULT_ERR(LEXER_ERR_INVAL_PARAM);

	fseek(in, 0, SEEK_SET);

	*lexer = (struct lexer *)malloc(sizeof(struct lexer));
	if (!*lexer)
		return LEXER_RESULT_ERR(LEXER_ERR_INTERNAL);

	(*lexer)->in = in;

	(*lexer)->tok_lines = NULL;
	(*lexer)->tok_lines_cap = 0;
	(*lexer)->tok_lines_len = 0;

	Lexer_Result result = s_resize_tok_lines(*lexer);

	if (!result.is_ok) {
		lexer_deinit(*lexer);
		free(*lexer);
		return LEXER_RESULT_ERR(LEXER_ERR_INTERNAL);
	}

	return LEXER_RESULT_OK;
}

static Lexer_Result
s_insert_char2lexeme(struct lexer_tok *tok, char ch)
{
	assert(tok);

	if (tok->lexeme_len == LEXER_MAX_LEXEME_LEN)
		return LEXER_RESULT_ERR(LEXER_ERR_INVAL_LEXEME_LEN);

	tok->lexeme[tok->lexeme_len++] = ch;

	return LEXER_RESULT_OK;
}

static inline bool
s_can_end_integer_tok(char ch)
{
	switch (ch) {
		case ' ':
		case '\n':
		case '(':
		case ',':
			return true;
	}
	return false;
}

static inline bool
s_can_end_identifier_tok(char ch)
{
	switch (ch) {
		case ',':
		case '\n':
		case ' ':
		case '(':
		case ')':
			return true;
	}

	return false;
}

static Lexer_Result
s_run_line(struct lexer_tok_line *tok_line, const char rd_line[],
	   size_t rd_line_len)
{
#define TOKS (tok_line->toks)
#define TOKS_CAP (tok_line->toks_cap)
#define TOKS_LEN (tok_line->toks_len)
#define TOK (TOKS[TOKS_LEN])

	assert(tok_line);
	assert(rd_line);
	assert(rd_line_len);

	enum LEXER_STATE_ state = LEXER_STATE_NORMAL;

	bool was_prev_char = false;

	for (size_t i = 0; i < rd_line_len; ++i) {
		if (TOKS_LEN == TOKS_CAP) {
			const Lexer_Result result = s_resize_toks(tok_line);
			LEXER_RET_IF_ERR(result);
		}

		char ch = rd_line[i];
		Lexer_Result result = LEXER_RESULT_OK;

		switch (state) {
			case LEXER_STATE_NORMAL:
				switch (ch) {
					case '\n':
						TOK.kind =
							LEXER_TOK_KIND_NEWLINE;
						TOKS_LEN++;
						goto END;
					case ' ':
						break;
					case ',':
						TOK.kind = LEXER_TOK_KIND_COMMA;
						TOKS_LEN++;
						break;
					case ':':
						TOK.kind = LEXER_TOK_KIND_COLON;
						TOKS_LEN++;
						break;
					case '(':
						TOK.kind =
							LEXER_TOK_KIND_L_PARENTHESES;
						TOKS_LEN++;
						break;
					case ')':
						TOK.kind =
							LEXER_TOK_KIND_R_PARENTHESES;
						TOKS_LEN++;
						break;
					case '\"':
						TOK.kind =
							LEXER_TOK_KIND_STRING;
						state = LEXER_STATE_STRING;
						break;
					case '\'':
						TOK.kind =
							LEXER_TOK_KIND_INTEGER;
						state = LEXER_STATE_ASCII;
						break;
					default:
						if (isdigit((
							    unsigned char)ch)) {
							TOK.kind =
								LEXER_TOK_KIND_INTEGER;
							state = LEXER_STATE_INTEGER;
						} else if (
							isalpha((unsigned char)
									ch) ||
							ch == '_') {
							TOK.kind =
								LEXER_TOK_KIND_IDENTIFIER;
							state = LEXER_STATE_IDENTIFIER;
						} else {
							return LEXER_RESULT_ERR(
								LEXER_ERR_INVAL_CHAR);
						}
						result = s_insert_char2lexeme(
							&TOK, ch);
						LEXER_RET_IF_ERR(result);
						break;
				}
				break;
			case LEXER_STATE_IDENTIFIER:
				if (isalpha((unsigned char)ch) ||
				    isdigit((unsigned char)ch) || ch == '_') {
					result = s_insert_char2lexeme(&TOK, ch);
					LEXER_RET_IF_ERR(result);
				} else if (s_can_end_identifier_tok(ch)) {
					TOKS_LEN++;
					i--;
					state = LEXER_STATE_NORMAL;
				} else {
					return LEXER_RESULT_ERR(
						LEXER_ERR_INVAL_CHAR);
				}
				break;
			case LEXER_STATE_STRING:
				switch (ch) {
					case '\"':
						TOKS_LEN++;
						state = LEXER_STATE_NORMAL;
						break;
					default:
						result = s_insert_char2lexeme(
							&TOK, ch);
						LEXER_RET_IF_ERR(result);
						break;
				}
				break;
			case LEXER_STATE_ASCII:
				if (ch == '\'') { /* Always first */
					was_prev_char = false;
					TOKS_LEN++;
					state = LEXER_STATE_NORMAL;
				} else if (IS_ASCII(ch)) {
					if (was_prev_char)
						return LEXER_RESULT_ERR(
							LEXER_ERR_INVAL_ASCII);
					was_prev_char = true;
					TOK.val.integer = (int64_t)ch;
				} else {
					return LEXER_RESULT_ERR(
						LEXER_ERR_INVAL_ASCII);
				}
				break;
			case LEXER_STATE_INTEGER:
				if (isdigit((unsigned char)ch)) {
					result = s_insert_char2lexeme(&TOK, ch);
					LEXER_RET_IF_ERR(result);
				} else if (s_can_end_integer_tok(ch)) {
					TOKS_LEN++;
					i--;
					state = LEXER_STATE_NORMAL;
				} else {
					return LEXER_RESULT_ERR(
						LEXER_ERR_INVAL_INTEGER);
				}
				break;
			default:
				assert(0);
				return LEXER_RESULT_ERR(LEXER_ERR_INTERNAL);
		}
	}

	if (state != LEXER_STATE_NORMAL)
		return LEXER_RESULT_ERR(LEXER_ERR_INVAL_SYNTAX);
END:
	return LEXER_RESULT_OK;

#undef TOK
#undef TOKS
#undef TOKS_CAP
#undef TOKS_LEN
}

Lexer_Result
lexer_run(struct lexer *lexer)
{
	if (!lexer)
		return LEXER_RESULT_ERR(LEXER_ERR_INVAL_PARAM);

	char line[PP_MAX_LINE_LEN + 1] = {
		0,
	};

	for (;;) {

		if (lexer->tok_lines_len == lexer->tok_lines_cap) {
			const Lexer_Result result = s_resize_tok_lines(lexer);
			LEXER_RET_IF_ERR(result);
		}

		if (!fgets(line, sizeof(line), lexer->in))
			break;

		const size_t line_len = strlen(line);
		if (!line_len)
			return LEXER_RESULT_ERR(LEXER_ERR_INTERNAL);
		else if (line[line_len - 1] != '\n')
			return LEXER_RESULT_ERR(LEXER_ERR_INVAL_LINE_LEN);

		const Lexer_Result result =
			s_run_line(&lexer->tok_lines[lexer->tok_lines_len],
				   line, line_len);
		if (!result.is_ok)
			return result;

		lexer->tok_lines_len++;
	}

	return feof(lexer->in) ? LEXER_RESULT_OK
			       : LEXER_RESULT_ERR(LEXER_ERR_INTERNAL);
}
