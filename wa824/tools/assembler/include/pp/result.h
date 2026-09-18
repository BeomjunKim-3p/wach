#ifndef WA824I_PP_RESULT_H
#define WA824I_PP_RESULT_H

#include <pp/err.h>
#include <stdbool.h>

typedef struct pp_result {
	bool is_ok;
	enum PP_ERR_ err;
} PP_Result;

#endif
