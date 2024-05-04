int printf(string fmt, ...);

int add(int a, int b, int k)
{
    int c;
    int i;
    i = 0;

    while (i < k) {
        i = i + 1;
        c = c + 1;
    }

    return a + b;
}

int main(int k)
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

    while (a < k) {
        c = 0;
        a = a + 1;
    }

    if (a > 0) {
        i = 0;
        j = 0;
    } 
    else {
        d = 100;
        e = 1;
    }

    b = 10;
    a = 0;
    while (a < k) {
        c = c * 1;
    }

    return e + add(0, 0, k);
}