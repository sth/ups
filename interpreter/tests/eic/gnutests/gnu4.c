void foo(void);
int bar(int i)
{}

struct s {int x[5];};

int main()
{
        struct s {struct s *next; int i;};
	static struct s sa[2];
 
	foo();
	printf("Sizeof struct s in main() = %d\n", sizeof(struct s));
	printf("Sizeof struct s.next->i = %d\n", sa[0].next->i);
	printf("Test passed (if it compiled)\n");
	return 0;
        /* bar(sa[0].next->x[4]); */
        bar(sa[0].next->i); 
}

void foo(void)
{
	printf("Sizeof struct s in foo() = %d\n", sizeof(struct s));
}	


#ifdef EiCTeStS
main();
#endif
