int printf(const char *,...);

int mystrlen(const char *s)
{
	int len;

	for (len = 0; *s != 0; len++, s++);
	return len;
}

int main(void)
{
	char *text = "This is a string.";

	printf("strlen(%s) = %d\n", text, strlen(text));
	printf("mystrlen(%s) = %d\n", text, mystrlen(text));

	return 0;
}

