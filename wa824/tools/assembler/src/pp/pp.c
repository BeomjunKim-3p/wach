#include <assert.h>
#include <pp/pp.h>
#include <stdlib.h>
#include <string.h>

enum PP_STATE_ {
	PP_STATE_NORMAL,
	PP_STATE_LINE_COMMENT,
	PP_STATE_WHITE_SPACE,
};

struct pp {
	FILE *in;
	FILE *out;
	enum PP_STATE_ state;
};

static PP_Result
s_run_line(struct pp *pp, const char rd_line[], size_t rd_line_len)
{
	assert(pp);
	assert(rd_line);
	assert(rd_line_len && rd_line_len <= PP_MAX_LINE_LEN);

	char wr_line[PP_MAX_LINE_LEN + 1];
	size_t wr_line_len = 0;

	size_t i;
	for (i = 0; i < rd_line_len; ++i) {
		switch (pp->state) {
		case PP_STATE_NORMAL:
			switch (rd_line[i]) {
			case '\r':
				break;
			case '\n':
				wr_line[wr_line_len] = '\n';
				wr_line_len++;
				break;
			case '\t':
				/* fall through */
			case ' ':
				wr_line[wr_line_len] = ' ';
				wr_line_len++;
				pp->state = PP_STATE_WHITE_SPACE;
				break;
			case ';':
				pp->state = PP_STATE_LINE_COMMENT;
				break;
			default:
				wr_line[wr_line_len] = rd_line[i];
				wr_line_len++;
				break;
			}
			break;
		case PP_STATE_LINE_COMMENT:
			switch (rd_line[i]) {
			case '\n':
				wr_line[wr_line_len] = '\n';
				wr_line_len++;
				/* fall through */
			case '\r':
				pp->state = PP_STATE_NORMAL;
				break;
			default:
				break;
			}
			break;
		case PP_STATE_WHITE_SPACE:
			switch (rd_line[i]) {
			case '\t':
				/* fall through */
			case ' ':
				break;
			default:
				pp->state = PP_STATE_NORMAL;
				i--;
				break;
			}
			break;
		default:
			assert(0);
			break;
		}
	}

	wr_line[wr_line_len - 1] = '\0';

	return EOF == fputs(wr_line, pp->out)
		       ? (PP_Result){.is_ok = false, .err = PP_ERR_INTERNAL}
		       : (PP_Result){.is_ok = true, .err = 0};
}

PP_Result
pp_init(struct pp **pp, FILE *in, FILE *out)
{
	if (!pp || !in || !out)
		return (PP_Result){.is_ok = false, .err = PP_ERR_INVAL_PARAM};

	*pp = (struct pp *)malloc(sizeof(struct pp));
	if (!*pp)
		return (PP_Result){.is_ok = false, .err = PP_ERR_INTERNAL};

	(*pp)->in = in;
	(*pp)->out = out;
	(*pp)->state = PP_STATE_NORMAL;

	fseek(in, 0, SEEK_SET);
	fseek(out, 0, SEEK_SET);

	return (PP_Result){.is_ok = true, .err = 0};
}

PP_Result
pp_run(INOUT_ struct pp *pp)
{
	char line[PP_MAX_LINE_LEN + 1] = {
		0,
	};

	while (fgets(line, sizeof(line), pp->in)) {
		const size_t line_len = strlen(line);

		if (!line_len)
			return (PP_Result){.is_ok = false,
					   .err = PP_ERR_INTERNAL};
		/* Every line must be finished with '\n' */
		if (line[line_len - 1] != '\n')
			return (PP_Result){.is_ok = false,
					   .err = PP_ERR_INVAL_LINE_LEN};

		const PP_Result ret = s_run_line(pp, line, line_len);
		if (!ret.is_ok)
			return ret;
	}

	return feof(pp->in)
		       ? (PP_Result){.is_ok = true, .err = 0}
		       : (PP_Result){.is_ok = false, .err = PP_ERR_INTERNAL};
}

PP_Result
pp_deinit(struct pp *pp)
{
	if (!pp)
		return (PP_Result){.is_ok = false, .err = PP_ERR_INVAL_PARAM};

	free(pp);

	return (PP_Result){.is_ok = true, .err = 0};
}
