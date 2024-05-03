void m1() {
    int a;
    int b;
    int c;
    int d;
    int e;

    for (a = 0; b = 1; a < 0; a = a + 1;) {
        c = 10;
    }

    for (a = 0; d = 1; a < 1000; a = a + 1;) {
        e = 10;
    }
}

int m2() {
    int a;
    int b;
    b = 10;

    for (a = 0; a < 1000; a = a + 1;) {
        b = b + 1;
    }

    return b;
}

int m3(int a) {
    int b;
    b = 10;

    for (a = 0; a < 1000; a = a + 1;) {
        b = b + 1;
    }

    return 0;
}

int main() {
    int a;
    int b;
    int c;
    int d;
    int e;

    a = 0;
    b = 10;
    c = a + b;

    for (a = 0; a < 1000; a = a + 1;) {
        b = m3(c);
        m2();
        c = c + 1;
    }

    while(a < 1000) {
        m1();
        e = m3(a);
        c = c + 1;
    }

    while(a < 1000) {
        m1();
        m3(a);
        c = c + 1;
    }

    d = c + a;
    return d;
}
