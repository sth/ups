extern int printf(const char *, ...);
extern int $add(int,...);

int main()
{
	printf("$add(1,2,3) returned %d\n", $add(1,2,3));
	return 0;
}
