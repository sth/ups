#include <stdio.h>
#include <assert.h>


int main(void)
{
    int v = 1;
    assert(v == 1);
    fprintf(stderr,"Assert should fail on line %d\n",__LINE__ + 1);
    assert(v == 0);
    return 0;
}
    
#ifdef EiCTeStS
main();
#endif
