#include <assert.h>
#include <limits.h>

int main()
{
	unsigned int u = 5;
	int i = INT_MAX;

	assert((-u)+u == 0);
	assert((-i)+i == 0);
	assert(0-i == (-i));
}
		
