/* example4.c */

#include "Cinterpreter.h"

enum { 
	ADD_FUNC = -1 
};

static ci_nametype_t
mygetaddr(name, p_addr)
	const char *name;
	taddr_t *p_addr;
{
	ci_nametype_t nametype;

	if (strcmp(name, "$add") == 0) {
		*p_addr = (taddr_t)ADD_FUNC;
		nametype = CI_INDIRECT_LIBFUNC;
	}
	else
		nametype = ci_standard_getaddr(name, p_addr);

	return nametype;
}

static ci_exec_result_t
call_external_function(ma, addr, args, nwords, p_res)
	machine_t *ma;
	taddr_t addr;
	taddr_t *args;
	int nwords;
	taddr_t *p_res;
{
	if (addr == (taddr_t)ADD_FUNC) {
		int i,j;
		for (i = 0, j = 0; i < nwords; i++) {
			j += (int)args[i];
		}
		*p_res = (taddr_t)j;
	} else {
		return  CI_ER_USER1;
	}
	return CI_ER_CONTINUE;
}


int main(void)
{
	Cinterpreter_t ci;

	if ( ci_create_interpreter_from_file(&ci, "add.c", NULL, mygetaddr) ) {
		ci_execute_interpreter(&ci, FALSE, call_external_function);
		ci_destroy_interpreter(&ci);
	}

	return 0;
}
