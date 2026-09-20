#ifndef WA824I_LEXER_RESULT_H
#define WA824I_LEXER_RESULT_H

#include <lexer/err.h>
#include <stdbool.h>

typedef struct lexer_result {
	bool is_ok;
	enum LEXER_ERR_ err;
} Lexer_Result;


#define LEXER_RESULT_ERR(err_) (Lexer_Result){.is_ok = false, .err = err_}
#define LEXER_RESULT_OK (Lexer_Result){.is_ok = true, .err = 0}


#define LEXER_RET_IF_ERR(result_) 		\
	do {				\
		if (!result.is_ok)	\
			return result;	\
	} while (0)


#endif
