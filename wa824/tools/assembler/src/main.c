#include <lexer/lexer.h>
#include <pp/pp.h>
#include <stdio.h>

int
main(int argc, char *argv[])
{

	if (argc != 3) {
		printf("[ERROR] Not enough arguments. $ %s <src_file> "
		       "<dst_file>\n",
		       argv[0]);
		return -1;
	}

	FILE *in = fopen(argv[1], "r");
	if (!in) {
		printf("[ERROR] Failed to open file <%s>\n", argv[1]);
		return -1;
	}

	FILE *out = fopen(argv[2], "r+");
	if (!in) {
		fclose(in);
		printf("[ERROR] Failed to open file <%s>\n", argv[2]);
		return -1;
	}

	/* -----------------------------pp--------------------------- */
	struct pp *pp = NULL;
	PP_Result pp_result;

	fseek(in, 0, SEEK_SET);
	fseek(out, 0, SEEK_SET);

	pp_result = pp_init(&pp, in, out);
	if (!pp_result.is_ok) {
		fclose(in);
		fclose(out);
		printf("[ERROR] Failed to initialize preprocessor. PP_ERR_ = "
		       "%d\n",
		       pp_result.err);
		return -1;
	}

	pp_result = pp_run(pp);
	if (!pp_result.is_ok) {
		pp_deinit(pp);
		fclose(in);
		fclose(out);
		printf("[ERROR] Failed to run preprocessor. PP_ERR_ = %d\n",
		       pp_result.err);
		return -1;
	}

	pp_deinit(pp);
	pp = NULL;

	/* ---------------------------- lexer ------------------------------ */

	struct lexer lexer;
	struct as_diag diag;
	Lexer_Result lexer_result;

	as_init_diag(&diag);
	fseek(out, 0, SEEK_SET);

	lexer_init(&lexer, out, &diag);

	lexer_result = lexer_run(&lexer);
	if (!lexer_result.is_ok) {
		lexer_deinit(&lexer);
		fclose(in);
		fclose(out);
		printf("[ERROR] Failed to run lexer. LEXER_ERR_ = %d\n",
		       lexer_result.err);
		return -1;
	}

	const struct lexer_tok_line *tok_lines = NULL;
	size_t tok_lines_len = lexer_get_tok_lines(&lexer, &tok_lines);
	if (!lexer_result.is_ok) {
		lexer_deinit(&lexer);
		fclose(in);
		fclose(out);
		printf("[ERROR] Failed to get tok_lines. LEXER_ERR_ = %d\n",
		       lexer_result.err);

		return -1;
	}

	for (size_t line_idx = 0; line_idx < tok_lines_len; ++line_idx) {
		const struct lexer_tok_line *curr_line = &tok_lines[line_idx];
		printf("LEXER_TOK_KIND_ = ");
		for (size_t tok_idx = 0; tok_idx < curr_line->toks_len;
		     ++tok_idx) {
			printf(" %d,", curr_line->toks[tok_idx].kind);
		}
		printf("\n");
	}

	lexer_deinit(&lexer);

	fclose(in);
	fclose(out);

	return 0;
}
