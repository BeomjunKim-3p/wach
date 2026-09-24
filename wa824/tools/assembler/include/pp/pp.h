#ifndef WA824I_PP_PP_H_
#define WA824I_PP_PP_H_

#include <stdio.h>
#include <pp/result.h>
#include <pp/def.h>
#include <mac.h>


struct pp;

PP_Result
pp_init(OUT_ struct pp ** ALLOC_ pp, INOUT_ FILE *in, OUT_ FILE *out);

PP_Result
pp_run(INOUT_ struct pp *pp);

void
pp_deinit(ALLOC_ TAKE_ struct pp *pp); 

#endif
