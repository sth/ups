
struct fields {
    unsigned int j:5;
    unsigned int k:4, l:6, :12, m:4, n:6;
    unsigned int s:7;
    unsigned char c:2;
    long long p:45;
};

int main()
{
    /*                  j     k    l     m    n     s     c    p */
    struct fields s = { 0x1f, 0x0, 0x21, 0x0, 0x1c, 0x41, 0x3, 0x987654abcdeUL };
    return 0;
}

