#include <stdio.h>
int exec_func(int func(int)) 
{
	return func(5);
}

int func(int i)
{
	return i+1;
}

int main()
{
	if (exec_func(func) != 6)
		printf("test_funcptr.c failed\n");
	return 0;
}
