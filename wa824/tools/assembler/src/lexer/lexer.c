#include <assert.h>
#include <lexer/lexer.h>
#include <lexer/result.h>
#include <pp/def.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_TOK_LINES_LEN 100
#define DEFAULT_TOKS_LEN 10

enum LEXER_STATE_ {
	LEXER_STATE_NORMAL,
	LEXER_STATE_IDENTIFIER,
	LEXER_STATE_STRING,
	LEXER_STATE_ASCII,
	LEXER_STATE_INTEGER,
};

static inline void
s_set_diag_msg(struct as_diag *diag, const char msg[])
{
	as_set_diag(diag, diag->line, diag->column, msg);
}

static inline bool
s_is_ascii(unsigned char ch)
{
	return ch <= 0x7Fu;
}

static inline bool
s_is_alpha(unsigned char ch)
{
	if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
		return true;
	return false;
}

static inline bool
s_is_digit(unsigned char ch)
{
	return ch >= '0' && ch <= '9';
}

static void
s_deinit_tok(struct lexer_tok *tok)
{
	assert(tok);

	tok->kind = LEXER_TOK_KIND_INIT;
	tok->lexeme_len = 0;
	tok->val.integer = 0;
	memset(tok->lexeme, 0, sizeof(tok->lexeme));
}

static void
s_deinit_tok_line(struct lexer_tok_line *tok_line)
{
	assert(tok_line);

	tok_line->toks_len = 0;

	if (!tok_line->toks) {
		tok_line->toks_cap = 0;
		return;
	}

	for (size_t i = 0; i < tok_line->toks_cap; ++i)
		s_deinit_tok(&tok_line->toks[i]);

	free(tok_line->toks);

	tok_line->toks_cap = 0;
}

void
lexer_deinit(struct lexer *lexer)
{
	assert(lexer);

	lexer->tok_lines_len = 0;

	if (!lexer->tok_lines) {
		lexer->tok_lines_cap = 0;
		return;
	}

	for (size_t i = 0; i < lexer->tok_lines_cap; ++i)
		s_deinit_tok_line(&lexer->tok_lines[i]);

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
	memset(tok->lexeme, 0, sizeof(tok->lexeme));
}

static Lexer_Result
s_resize_toks(struct lexer_tok_line *tok_line, struct as_diag *diag)
{
	assert(tok_line && diag);

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

	if (!tmp_ptr) {
		s_set_diag_msg(diag, "Out of memory\n");
		return LEXER_RESULT_ERR(LEXER_ERR_INTERNAL);
	}

	tok_line->toks = tmp_ptr;

	for (size_t i = 0; i < DEFAULT_TOKS_LEN; ++i)
		s_init_tok(&tok_line->toks[tok_line->toks_cap + i]);

	tok_line->toks_cap += DEFAULT_TOKS_LEN;

	return LEXER_RESULT_OK;
}
static void
s_init_tok_line(struct lexer_tok_line *tok_line)
{
	assert(tok_line);

	tok_line->toks = NULL;
	tok_line->toks_len = 0;
	tok_line->toks_cap = 0;
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

	if (!tmp_ptr) {
		s_set_diag_msg(lexer->diag, "Out of memory");
		return LEXER_RESULT_ERR(LEXER_ERR_INTERNAL);
	}

	lexer->tok_lines = tmp_ptr;

	for (size_t i = 0; i < DEFAULT_TOK_LINES_LEN; ++i)
		s_init_tok_line(&lexer->tok_lines[lexer->tok_lines_cap + i]);

	lexer->tok_lines_cap += DEFAULT_TOK_LINES_LEN;

	return LEXER_RESULT_OK;
}

size_t
lexer_get_tok_lines(struct lexer *lexer,
		    const struct lexer_tok_line **tok_lines)
{
	assert(lexer && tok_lines);

	*tok_lines = lexer->tok_lines;
	return lexer->tok_lines_len;
}

void
lexer_init(struct lexer *lexer, FILE *in, struct as_diag *diag)
{
	assert(lexer && in && diag);

	lexer->diag = diag;

	lexer->in = in;
	lexer->tok_lines = NULL;
	lexer->tok_lines_cap = 0;
	lexer->tok_lines_len = 0;
}

static Lexer_Result
s_insert_char2lexeme(struct lexer_tok *tok, char ch, struct as_diag *diag)
{
	assert(tok);

	if (tok->lexeme_len == TOK_MAX_LEXEME_LEN) {
		s_set_diag_msg(diag, "Too long lexeme length\n");
		return LEXER_RESULT_ERR(LEXER_ERR_INVAL_LEXEME_LEN);
	}

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
	   size_t rd_line_len, struct as_diag *diag)
{
#define TOKS (tok_line->toks)
#define TOKS_CAP (tok_line->toks_cap)
#define TOKS_LEN (tok_line->toks_len)
#define TOK (TOKS[TOKS_LEN])

	assert(tok_line && rd_line && diag);
	assert(rd_line_len);

	enum LEXER_STATE_ state = LEXER_STATE_NORMAL;

	bool was_prev_char = false;

	for (size_t i = 0; i < rd_line_len; ++i, ++diag->column) {
		if (TOKS_LEN == TOKS_CAP)
			LEXER_RET_IF_ERR(s_resize_toks(tok_line, diag));

		char ch = rd_line[i];

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
						if (s_is_digit((
							    unsigned char)ch)) {
							TOK.kind =
								LEXER_TOK_KIND_INTEGER;
							state = LEXER_STATE_INTEGER;
						} else if (
							s_is_alpha(
								(unsigned char)
									ch) ||
							ch == '_' ||
							ch == '$') {
							TOK.kind =
								LEXER_TOK_KIND_IDENTIFIER;
							state = LEXER_STATE_IDENTIFIER;
						} else {
							s_set_diag_msg(
								diag,
								"Invalid "
								"character"
								"\n");
							return LEXER_RESULT_ERR(
								LEXER_ERR_INVAL_CHAR);
						}
						LEXER_RET_IF_ERR(
							s_insert_char2lexeme(
								&TOK, ch,
								diag));
						break;
				}
				break;
			case LEXER_STATE_IDENTIFIER:
				if (s_is_alpha((unsigned char)ch) ||
				    s_is_digit((unsigned char)ch) ||
				    ch == '_' || ch == '$') {
					LEXER_RET_IF_ERR(s_insert_char2lexeme(
						&TOK, ch, diag));
				} else if (s_can_end_identifier_tok(ch)) {
					TOKS_LEN++;
					i--;
					diag->column--;
					state = LEXER_STATE_NORMAL;
				} else {
					s_set_diag_msg(diag,
						       "Invalid identifier\n");
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
						LEXER_RET_IF_ERR(
							s_insert_char2lexeme(
								&TOK, ch,
								diag));
						break;
				}
				break;
			case LEXER_STATE_ASCII:
				if (ch == '\'') { /* Always first */
					was_prev_char = false;
					TOKS_LEN++;
					state = LEXER_STATE_NORMAL;
				} else if (s_is_ascii(ch)) {
					if (ch == '\n') {
						s_set_diag_msg(
							diag,
							"Invalid ascii: Ascii "
							"can't store "
							"\'\\n\'\n");
						return LEXER_RESULT_ERR(
							LEXER_ERR_INVAL_ASCII);
					}
					if (was_prev_char) {
						s_set_diag_msg(
							diag,
							"Invalid ascii: "
							"Too long ascii\n");
						return LEXER_RESULT_ERR(
							LEXER_ERR_INVAL_ASCII);
					}

					was_prev_char = true;
					TOK.val.integer = (int64_t)ch;
				} else {
					s_set_diag_msg(diag, "Invalid ascii\n");
					return LEXER_RESULT_ERR(
						LEXER_ERR_INVAL_ASCII);
				}
				break;
			case LEXER_STATE_INTEGER:
				if (s_is_digit((unsigned char)ch)) {
					LEXER_RET_IF_ERR(s_insert_char2lexeme(
						&TOK, ch, diag));
				} else if (s_can_end_integer_tok(ch)) {
					char *end_ptr = NULL;
					TOK.val.integer = strtoll(TOK.lexeme,
								  &end_ptr, 10);
					if (*end_ptr) {
						s_set_diag_msg(
							diag,
							"!!!!Internal!!!!: "
							"Invalid integer\n");
						return LEXER_RESULT_ERR(
							LEXER_ERR_INTERNAL);
					}
					TOKS_LEN++;
					i--;
					diag->column--;
					state = LEXER_STATE_NORMAL;
				} else {
					s_set_diag_msg(diag,
						       "Invalid integer\n");
					return LEXER_RESULT_ERR(
						LEXER_ERR_INVAL_INTEGER);
				}
				break;
			default:
				assert(0);
				s_set_diag_msg(diag,
					       "!!!!Internal!!!!: Invalid "
					       "<LEXER_STATE_>\n");
				return LEXER_RESULT_ERR(LEXER_ERR_INTERNAL);
		}
	}
END:
	assert(state == LEXER_STATE_NORMAL);

	return LEXER_RESULT_OK;

#undef TOK
#undef TOKS
#undef TOKS_CAP
#undef TOKS_LEN
}

Lexer_Result
lexer_run(struct lexer *lexer)
{
	assert(lexer);

	char line[PP_MAX_LINE_LEN + 1] = {
		0,
	};

	for (;;) {

		if (lexer->tok_lines_len == lexer->tok_lines_cap)
			LEXER_RET_IF_ERR(s_resize_tok_lines(lexer));

		if (!fgets(line, sizeof(line), lexer->in))
			break;

		const size_t line_len = strlen(line);
		if (!line_len) {
			s_set_diag_msg(lexer->diag,
				       "!!!!Internal!!!!: Zero length line\n");

			return LEXER_RESULT_ERR(LEXER_ERR_INTERNAL);
		} else if (line[line_len - 1] != '\n') {
			s_set_diag_msg(
				lexer->diag,
				"Invalid line length: Too long line length\n");
			return LEXER_RESULT_ERR(LEXER_ERR_INVAL_LINE_LEN);
		}

		LEXER_RET_IF_ERR(
			s_run_line(&lexer->tok_lines[lexer->tok_lines_len],
				   line, line_len, lexer->diag));

		lexer->tok_lines_len++;
		lexer->diag->line++;
	}

	return feof(lexer->in) ? LEXER_RESULT_OK
			       : LEXER_RESULT_ERR(LEXER_ERR_INTERNAL);
}
