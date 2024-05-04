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
    a = 1; // Variable dead through one dataflow path only
    if(condition) {
        a = 2;
    }
    printf("%f\n", a);
    b = 3; // Variable dead through all dataflow paths
    if(condition) {
        b = 4; // Dead assignment in if statement
    }
    b = 5;
    printf("%f\n", b);
    c = 6; // Variable dead through one dataflow path only
    if(condition) {}
    else {
        c = 7;
    }
    printf("%f\n", c);
    d = 8; // Variable dead through all dataflow paths
    if(condition) {
        d = 9;
    }
    else {
        d = 10;
    }
    printf("%f\n", d);
    e = 11; // Variable dead through for loop body only
    for(i = 0; i < x; i = i + 1;) {
        e = 12;
    }
    printf("%f\n", e);
    f = 13; // Variable dead on all paths through for loop
    for(i = 0; i < x; i = i + 1;) {
        f = 14; // Dead assignment in for loop
    }
    f = 15;
    printf("%f\n", f);
    g = 16; // Variable dead through while loop body only
    i = 0;
    while(i < x) {
        e = 17;
        i = i + 1;
    }
    printf("%f\n", g);
    h = 18; // Variable dead on all paths through while loop
    i = 0;
    while(i < x) {
        h = 19; // Dead assignment in while loop
        i = i + 1;
    }
    h = 20;
    printf("%f\n", h);
    return 0;
}