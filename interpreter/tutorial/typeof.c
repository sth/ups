extern int printf(const char *, ...);
extern const char *$typename();

struct name {
	char firstname[30];
	char lastname[30];
};	

int main()
{
	int i;
	long l;
	struct name n;

	printf("\n");
	printf("int         i;\n");
	printf("long        l;\n");
	printf("struct name n;\n");
	printf("$typename(i)      = %s\n", $typename(i));
	printf("$typename(l)      = %s\n", $typename(l));
	printf("$typename(&l)     = %s\n", $typename(&l));
	printf("$typename(n)      = %s\n", $typename(n));
	printf("$typename(&n)     = %s\n", $typename(&n));
	printf("$typename(printf) = %s\n", $typename(printf));
	return 0;
}
