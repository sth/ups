/* From Harbison & Steele */
#include <assert.h>
void dummyfunc(int i) {}
void testfunc(void handler(int), char arry[10])
{
	int intarry[5];
	int i = sizeof(struct s { unsigned bit: 1; });
	struct s var;
	int j = i;

	assert(sizeof(struct s) == sizeof var);
	assert(sizeof('c') == sizeof(int));
	assert(sizeof(char) == 1);
	assert(sizeof(int) == sizeof(unsigned));
	assert(sizeof(handler) == sizeof(void *));
	assert(sizeof arry == sizeof(char *));
	assert(sizeof(intarry) == sizeof(int)*5);
	assert(sizeof(intarry[0]) == sizeof(int));
	assert(sizeof(i++) == sizeof(int));
	assert(i == j);
}

int main()
{
	char arry[10];
	testfunc(dummyfunc, arry);
}
		
	
