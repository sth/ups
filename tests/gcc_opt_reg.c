
typedef struct {
    int x;
    char *s;
    int y;
} ST;

char * rtn (ST *p, char *d)
{
    register ST *q;
    register char *e;

    q = p;
    return (q->s);
}

int main (int argc, char **argv)
{
    ST st;
    char * res;

    st.s = argv[0];
    res = rtn (&st, argv[0]);
    exit (0);
}

