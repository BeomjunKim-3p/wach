#ifndef WA824I_PP_RESULT_H_
#define WA824I_PP_RESULT_H_

#include <pp/err.h>
#include <stdbool.h>

typedef struct pp_result {
	bool is_ok;
	enum PP_ERR_ err;
} PP_Result;

#define PP_RESULT_ERR(err_) (PP_Result){.is_ok = false, .err = err_}
#define PP_RESULT_OK (PP_Result){.is_ok = true, .err = 0}


#define PP_RET_IF_ERR(result_) 		\
	do {				\
		if (!result.is_ok)	\
			return result;	\
	} while (0)


#endif
