#include <stdio.h>


enum { CHAR, INT, DOUBLE };

enum rgb { RED, GREEN = 13, BLUE };

enum {Ed = 1, CLAIRE, SARAH = 1, ALEX, GEORGIA};

enum Rem;
enum b { abc };

extern enum rgb colour;


int main()
{
    enum rgb colour = RED;
#ifdef __CXC__
    enum rgb RGB = (enum rgb)5;
#else
    enum rgb RGB = 5;
#endif
    {    int shep = 13;
        {
	    enum {farm, axe=3, shep};
	    printf("shep = %d :-> shep = 4\n",shep);
	}
	 printf("shep = %d :-> shep = 13\n",shep);
     }    
    printf("colour = %d\n",colour);
    printf("RGB = %d\n",RGB);
    printf("Ed = %d, CLAIRE = %d, SARAH = %d, ALEX %d, GEORGIA = %d\n",
	   Ed,CLAIRE, SARAH,ALEX, GEORGIA);
    printf ("RED = %d,GREEN = %d,BLUE = %d "
	    ":-> RED = 0,GREEN = 13,BLUE = 14  \n", RED, GREEN, BLUE);
    printf ("CHAR = %d,INT = %d,DOUBLE = %d "
	    ":-> CHAR = 0,INT = 1,DOUBLE = 2 \n", CHAR, INT, DOUBLE);

    return 0;
}

#ifdef EiCTeStS
main();
#endif



