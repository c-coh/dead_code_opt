int printf(string fmt, ...);

int add(int a, int b, int k)
{
    int y;
    int z;
    z = 0;

    while (z < k) {
        z = z + 1;
        y = y + 1;
    }

    return a + b;
}

int main()
{
    int a;
    int b;
    int c;
    int d;
    int e;
    int i;
    int j;

    a = 0;
    b = 0;
    c = 0;
    d = 0;

    while (a < 100000) {
        c = 0;
        a = a + 1;
    }

    if (a > 0) {
        i = 0;
        i = j + 1;
        i = 2;
        j = 0;
        j = 3;
    } 
    else {
        d = 100;
        e = 1;
    }

    b = 10;
    a = 0;
    while (c < 100000) {
        c = c * 1;
        add(0, 0, 100000);
        c = c + 2;
    }

    return e + add(0, 0, 10);
}