/* example2.c */

#include "Cinterpreter.h"

int main()
{
	Cinterpreter_t ci;

	if ( ci_create_interpreter_from_file(&ci, "hello.c", NULL, NULL) ) {
		ci_execute_interpreter(&ci, FALSE, NULL);
		ci_destroy_interpreter(&ci);
	}

	return 0;
}
