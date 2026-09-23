#ifndef WA824I_AS_ERR_H_
#define WA824I_AS_ERR_H_

#include <mac.h>
#include <stddef.h>

#define AS_MAX_DIAG_MSG_LEN 100

struct as_diag {
	size_t line;
	size_t column;
	char msg[AS_MAX_DIAG_MSG_LEN + 1];
};

/*
 * @param msg nul terminated string
 * */
void as_set_diag(OUT_ struct as_diag *diag, size_t line, size_t column,
		 const char msg[]);

void as_init_diag(OUT_ struct as_diag *diag);

#endif
