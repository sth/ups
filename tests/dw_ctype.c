
#include <ctype.h>

int main()
{
    char *str = "qwertyuiop!'$%^&*()_+0987654321";
    char *ptr;

    ptr = str;
    while (*ptr != '\0') {
	if (isalpha(*ptr))
	    printf("%c", toupper(*ptr));
	else
	    printf("%c", *ptr);
	if (isalpha(*ptr))
	    printf("%c", toupper(*ptr));
	else
	    printf("%c", *ptr);
	ptr++;
    }
    printf("\n");
    return 0;
}
