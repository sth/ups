enum bool { FALSE, TRUE };
typedef enum bool bool;

int printf(const char *, ...);

void bool2str(bool value)
{
	if (value == TRUE) {
		printf("TRUE\n");
	}
	else if (value == FALSE) {
		printf("FALSE\n");
	} 
	else {
		printf("%d IS NOT A BOOLEAN VALUE\n", value);
	}
}

int main(void)
{
	bool2str(TRUE);
	bool2str(FALSE);
	bool2str((bool)5);
}
