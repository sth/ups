#include <stdio.h>

struct tree {
	struct tree *left;
	struct tree *right;
};

struct tree * assign_to(struct tree *s)
{
/* struct tree *left = s->left, *right = s->right; */
	struct tree *left, *right;
	left = s->left; right = s->right;

	printf("left = %ld, right = %ld\n", left, right);
	return left;
}

int main(void)
{
	struct tree l,r,t;
	struct tree *dummy;

	t.left = &l;
	t.right = &r;

	printf("left = %ld, right = %ld\n", t.left, t.right);
	dummy = assign_to(&t);
	return 0;
}


