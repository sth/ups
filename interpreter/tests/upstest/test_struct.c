#include <stdio.h>
struct MyStruct
{
	int dummy;
	int flags;
	char third_field;
};
typedef struct MyStruct mystruct_t;

mystruct_t 
retstruct(int i, int j)
{
	mystruct_t me;
	me.dummy = i;
	me.flags = j;
	return me;
}

int 
testret(int i)
{
	return i+1;
}

void
testvoid(void)
{
}


void main()
{
	int i = 0;
	mystruct_t me;
	me = retstruct(i, i+1);
	me.dummy += 1;

	retstruct(5,6);

	i = testret(i);
	testvoid();

	printf("me.dummy = %d, me.flags = %d\n", me.dummy, me.flags);
}
