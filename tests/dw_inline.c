/*
gcc31 -g -Wall -O -o dw_inline dw_inline.c
*/

static __inline__ int sq(int x)
{
    return x * x;
}

int main()
{
    int j, k, l;

    k = 4;
    j = sq(k);
    l = sq(j);
    printf ("j = %d, k = %d, l = %d\n", j, k, l);
    return 0;
}
