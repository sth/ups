/* example3.c */

#include "Cinterpreter.h"

/* code to be executed */
char *myprg[] = {
	"extern int printf(const char *, ...);",
	"int i = 5;",
	"void start(void)",
	"{",
	"	printf(\"i = %d\\n\", i);",
	"	i++;",
	"}",
	NULL
};
	
int main(void)
{
	Cinterpreter_t ci;

	if ( ci_create_interpreter(&ci, myprg, NULL) ) {

		/* The FALSE argument indicates that we do not want to
		 * initialise static data. This is not necessary because
		 * ci_create_interpreter() would have initialised anyway.
		 */
		ci_execute_interpreter(&ci, FALSE, NULL);

		/* Following call demonstrates how the code can
		 * be re-executed with static data retained from the previous
		 * execution.
		 */
		ci_execute_interpreter(&ci, FALSE, NULL);

		/* This time we reset static data, and re-execute.
		 */
		ci_execute_interpreter(&ci, TRUE, NULL);
		ci_destroy_interpreter(&ci);
	}

	return 0;
}
