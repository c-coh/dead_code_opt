int printf(string fmt, ...);

int add(int a, int b)
{
    int c;
    int i;
    int j;

    i = 0;

    while (i < 10000) {
        i = i + 1;
        c = c + 1;

        j = 0;

        while (j < 10000) {
            j = j + 1;
            c = c - 1;
        }
    }

    return a + b + c;
}

int nothing(int a, int b)
{
    int c;
    int i;
    int j;
    i = 0;

    while (i < 1000) {
        i = i + 1;
        c = c + 1;

        j = 0;

        while (j < 1000) {
            j = j + 1;
            c = c - 1;
        }
    }

    return 0;
}

int main()
{
    int a;
    int b;
    int c;
    int d;
    int e;
    int f;
    int i;
    int j;

    a = 0;
    b = 0;
    c = 0;
    d = 0;
    f = 0;

    while (a < 100000) {
        c = 0;
        a = a + 1;

        while (c < 100000) {
            b = b * 2;
            j = 0;
            b = b;
            c = c + 1;
        }
    }

    if (a > 0) {
        i = 0;
        j = 0;

        while (i < 10000) {
            j = 0;
            a = 1;
            a = 1;
            a = 1;
            i = i + 1;

            while (j < 10000) {
                b = b * 3;
                c = 0;
                c = 0;
                c = 0;
                b = b - 2;
                j = j + 1;
            }
        }
    } 
    else {
        d = 100;
        e = 1;
        c = 0;
    }

    b = 10;

    if (b > 10) {
        f = 1;
    }
    else {
        e = 1;
    }    
    
    a = 0;
    while (a < 10000) {
        c = c * 1;
        a = a + 1;
        c = 0;
        c = 2;
    }

    return e + add(0, 0) + nothing(0, 0);
}