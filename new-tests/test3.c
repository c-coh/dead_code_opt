int printf(string fmt, ...);

int main(bool condition, int x) {
    int a;
    int b;
    int c;
    int d;
    int e;
    int f;
    int g;
    int h;
    int i;
    a = 1;
    if(condition) {
        a = 2;
    }
    printf("%f\n", a);
    b = 3;
    if(condition) {
        b = 4;
    }
    b = 5;
    printf("%f\n", b);
    c = 6;
    if(condition) {}
    else {
        c = 7;
    }
    printf("%f\n", c);
    d = 8;
    if(condition) {
        d = 9;
    }
    else {
        d = 10;
    }
    printf("%f\n", d);
    e = 11;
    for(i = 0; i < x; i = i + 1;) {
        e = 12;
    }
    printf("%f\n", e);
    f = 13;
    for(i = 0; i < x; i = i + 1;) {
        f = 14;
    }
    f = 15;
    printf("%f\n", f);
    g = 16;
    i = 0;
    while(i < x) {
        e = 17;
        i = i + 1;
    }
    printf("%f\n", g);
    h = 18;
    i = 0;
    while(i < x) {
        h = 19;
        i = i + 1;
    }
    h = 20;
    printf("%f\n", h);
    return 0;
}