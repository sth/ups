/* example5.c 
* This example shows how the to extend the Interpreter. A new function called
* $typename() is defined - it can be called from interpreted code. Any 
* variable can be passed to it - the function will return the type of the
* variable expressed in English.
*/

#include "Cinterpreter.h"

enum { 
	TYPENAME_FUNC = -1 
};

static ci_nametype_t
mygetaddr(name, p_addr)
	const char *name;
	taddr_t *p_addr;
{
	ci_nametype_t nametype;

	if (strcmp(name, "$typename") == 0) {
		*p_addr = (taddr_t)TYPENAME_FUNC;
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
	if (addr == (taddr_t)TYPENAME_FUNC) {
		if (nwords) {
			type_t *type = (type_t*) args[nwords]; 	
			static char *str = NULL;
			
			if (str != NULL)
				free(str);
			str = ci_type_to_english(type, FALSE);
			*p_res = (taddr_t)str;
			return CI_ER_CONTINUE;
		}
	}
	return CI_ER_USER1;
}


int main(void)
{
	Cinterpreter_t ci;

	if ( ci_create_interpreter_from_file(&ci, "typeof.c", NULL, 
			mygetaddr) ) {

		ci_execute_interpreter(&ci, FALSE, call_external_function);
		ci_destroy_interpreter(&ci);
	}

	return 0;
}
