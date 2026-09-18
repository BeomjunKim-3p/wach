#ifndef WA824I_PP_PP_H
#define WA824I_PP_PP_H

#include <stdio.h>
#include <pp/result.h>
#include <mac.h>

#define PP_MAX_LINE_LEN 255

struct pp;

PP_Result
pp_init(OUT_ struct pp ** ALLOC_ pp, INOUT_ FILE *in, OUT_ FILE *out);

PP_Result
pp_run(INOUT_ struct pp *pp);

PP_Result
pp_deinit(ALLOC_ TAKE_ struct pp *pp); 

#endif
