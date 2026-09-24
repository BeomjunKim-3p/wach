#include <as/err.h>
#include <assert.h>
#include <string.h>

void
as_set_diag(OUT_ struct as_diag *diag, size_t line, size_t column,
	    const char msg[])
{
	assert(diag);
	assert(msg);

	diag->line = line;
	diag->column = column;
	memset(diag->msg, 0, sizeof(diag->msg));
	strncpy(diag->msg, msg, sizeof(diag->msg));
	assert(diag->msg[sizeof(diag->msg) - 1]); /* Too long message */
}

void
as_init_diag(OUT_ struct as_diag *diag)
{
	diag->line = 0;
	diag->column = 0;
	memset(diag->msg, 0, sizeof(diag->msg));
}