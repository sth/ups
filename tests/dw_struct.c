
typedef struct my_s {
    int a;
    int b;
    float d;
    struct in_s {
	char *ptr;
    } in;
} my;

void
fred(x)
my x;
{
    my w;
    w = x;
    w.b = 456;
    x.b = 789;
}

void
joe(y)
my *y;
{
    my z;
    z = *y;
    z.b = 123;
    y->b = 765;
}

int
main()
{
    my one, two;

    one.a = 1;
    one.b = 2;
    one.d = 3.14159;

    two.a = 2;
    two.b = 4;
    two.d = 0.12345;

    fred(one);
    joe(&two);

    return 0;
}

