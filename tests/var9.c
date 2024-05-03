int m(int a) {
    for (int i = 0; i < 5; i++) {
        a = 10;
    }
    for (int i = 0; i < 0; i++) {
        int b = 10;
        return b;
    }    
    return a;
}