#ifndef WA824I_PRRSER_RESULT_H_
#define WA824I_PRRSER_RESULT_H_


#include <parser/err.h>
#include <stdbool.h>

typedef struct parser_result {
	bool is_ok;
	enum PARSER_ERR_ err;
} Parser_Result;


#define PARSER_RESULT_ERR(err_) (Parser_Result){.is_ok = false, .err = err_}
#define PARSER_RESULT_OK (Parser_Result){.is_ok = true, .err = 0}


#define PARSER_RET_IF_ERR(result_) 	\
	do {				\
		if (!result.is_ok)	\
			return result;	\
	} while (0)



#endif
