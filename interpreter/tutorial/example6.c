/* example6.c
* This is another example of how the Interpreter can be extended. A function
* $typeof() is defined. This function returns the type structure associated
* with a variable. 
*/

#include "Cinterpreter.h"

enum { 
	TYPEOF_FUNC = -1 
};

static ci_nametype_t
mygetaddr(name, p_addr)
	const char *name;
	taddr_t *p_addr;
{
	ci_nametype_t nametype;

	if (strcmp(name, "$typeof") == 0) {
		*p_addr = (taddr_t)TYPEOF_FUNC;
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
	if (addr == (taddr_t)TYPEOF_FUNC) {
		if (nwords >= 1) {
			type_t *type = (type_t*) args[nwords]; 	
			*p_res = (taddr_t)type;
			return CI_ER_CONTINUE;
		}
	}
	return  CI_ER_USER1;
}


int main(void)
{
	Cinterpreter_t ci;

	if ( ci_create_interpreter_from_file(&ci, "typeof2.c", NULL, 
			mygetaddr) ) {

		ci_execute_interpreter(&ci, FALSE, call_external_function);
		ci_destroy_interpreter(&ci);
	}

	return 0;
}
