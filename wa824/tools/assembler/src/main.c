#include <stdio.h>
#include <pp/pp.h>


int main(int argc, char *argv[])
{

	if (argc != 3) {
		printf("[ERROR] Not enough arguments. $ %s <src_file> <dst_file>\n", argv[0]);
		return -1;
	}

	FILE *in = fopen(argv[1], "r");
	if (!in) {
		printf("[ERROR] Failed to open file <%s>\n", argv[1]);
		return -1;
	}
		
	FILE *out = fopen(argv[2], "w");
	if (!in) {
		printf("[ERROR] Failed to open file <%s>\n", argv[2]);
		return -1;
	}

	struct pp *pp;	
	PP_Result tmp;

	tmp = pp_init(&pp, in, out);
	if (!tmp.is_ok) {
		printf("[ERROR] Failed to initialize preprocessor. PP_ERR_ = %d\n", tmp.err);
	}

	tmp = pp_run(pp);
	if (!tmp.is_ok) {
		printf("[ERROR] Failed to run preprocessor. PP_ERR_ = %d\n", tmp.err);
	}

	pp_deinit(pp);
	pp = NULL;

	return 0;
}
