int printf(string fmt, ...);

float add(int a, float b)
{
    return a + b;
}

int main()
{
    int a;
    float b;
    float c;
    int s;
    a = 1;
    b = a; // Variable use in unary expression (int to float)
    c = b; // Variable use in assignment
    d = 2;
    if(b > c) {} // Variable use in binary expression (comparison)
    printf("%f\n", add(e, c)); // Variable use in function call
    a = 1;
    b = 1.0;
    c = 2.3;
    d = 4;
    return d; // Variable use in return statement
}