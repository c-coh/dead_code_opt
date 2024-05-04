int printf(string fmt, ...);

int main() {
    int a;
    int b;
    int c;
    int d;
    int e;
    int f;
    float g;
    int h;
    int i;
    a = 0; // Dead assignment in statement block
    c = 1 + (b = 0); // Dead assignment in binary expression (addition)
    e = d = 2; // Dead assignment in assignment statement
    g = f = 3; // Dead assignment in unary operator (int to float)
    printf("%f\n", h = 4); // Dead assignment in function call
    return i = 5; // Dead assignment in return statement
}