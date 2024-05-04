int m() {
    int a;
    int i;
    int b;
    a = 10;
    for (i = 0; i < 5; i = i + 1;) 
    {
        a = 10;
    }
    for (i = 0; i < 0; i = i + 1;) 
    {
        b = 10;
        return a;
    }    
    return a;
}