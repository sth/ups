#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	FILE *file = fopen("test_stdout.c", "r");

	if (! file) {
		perror("fopen");
		exit(1);
	}

	{
		char buffer[128];
		int count = 0;
		while (fgets(buffer, sizeof buffer, file)) {
			count++;
		}

		fprintf(stdout, "%d lines in test_stdout.c\n", count);
	}
	fclose(file);
}	
