typedef enum boolean bool;
enum boolean { FALSE, TRUE };

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

static bool bval1 = 1;
static enum boolean bval2 = 10;
bool bval3 = FALSE;
int ival1 = TRUE;

int main(void)
{
	bool2str(TRUE);
	bool2str(FALSE);
	bool2str(5);
	ival1 = TRUE+1;
	ival1 = TRUE ? bval2 : (bval2 != 10);
	if (ival1 != 10)
		printf("test_enum.c failed\n");
}
