#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "avl3.h"

typedef struct word_st {
	char str[31];
} word_t;

int compareword(void *key, void *object)
{
	word_t *w;

	w = (word_t*) object;

	return strcmp((char *)key,w->str);
}

void createword(void *object, void *key)
{
	word_t *w;

	w = (word_t*) object;

	strncpy(w->str, (char *)key, sizeof w->str - 1);
	w->str[sizeof w->str - 1] = 0;
}

void assignword(void *dst, void *src)
{
	word_t *w;

	w = (word_t*) src;
	createword(dst, w->str);
}

void destroyword(void *object)
{
}
	
	
AVL_vtbl vtab = { compareword, createword, assignword, destroyword };

int load_words(AVLTree *T, const char *file)
{
	FILE *fp;
	word_t w;
	char line[80];
	int i;


	fp = fopen(file, "r");
	if (fp == NULL) {
		perror("fopen");
		return 0;
	}

	i = 0;
	while (fgets(line, sizeof line, fp)) {

		int len;

		len = strlen(line);
		if (line[len-1] == '\n')
			line[len-1] = 0;


		AVLTree_Insert(T, line);
		i++;
	}

	fclose(fp);
	printf("%d objects inserted\n", i);

	return i;
}

int count_words_from_left(AVLTree *T, int num)
{
	word_t *w;
	int count;

	for (count = 0, w = AVLTree_FindFirst(T); 
	     w != NULL; 
	     w = AVLTree_FindNext(T,w)) {
		count++;
		printf("%d: %s\n", count, w->str);
	}

	if (num == count) {
		printf("verify left count OK\n");
	}
	else {
		printf("verify left count FAILED\n");
		return 0;
	}
	return num;
}

int count_words_from_right(AVLTree *T, int num)
{
	word_t *w;
	int count;

	for (count = 0, w = AVLTree_FindLast(T); 
	     w != NULL; 
	     w = AVLTree_FindPrevious(T,w)) {
		count++;
	}

	if (num == count) {
		printf("verify right count OK\n");
	}
	else {
		printf("verify right count FAILED\n");
		return 0;
	}
	return num;
}
	

int main(void)
{
	const char *file = "wordlist";
	int num = 0;
	AVLTree *T;

	T = AVLTree_New(&vtab, sizeof(word_t), 100);
	if (T == NULL) {
		printf("failed to create a tree\n");
		exit(1);
	}

	num = load_words(T, file);

	if (num) {
		num = count_words_from_left(T,num);
	}

	if (num) {
		num = count_words_from_right(T,num);
	}

	return 0;
}
