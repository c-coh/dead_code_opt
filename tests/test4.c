int printf(string fmt, ...);

int main(bool a) {
    int i;

    if(!false || !false) {
        printf("The condition is always true, so this code is reachable!");
    }
    else {
        printf("The condition is always true, so this code is unreachable!");
    }

    if(false && false) {
        printf("The condition is always false, so this code is unreachable!");
    }
    else {
        printf("The condition is always false, so this code is reachable!");
    }

    if(a) {
        printf("The condition is indeterminate, so this code is reachable!");
    }
    else {
        printf("The condition is indeterminate, so this code is reachable!");
    }

    while(!false || false) {
        printf("The condition is always true, so this code is reachable!");
    }

    while(!false && false) {
        printf("The condition is always false, so this code is unreachable!");
    }

    while(a || false) {
        printf("The condition is indeterminate, so this code is reachable!");
    }

    for(i = 0; !false && !false; i = i + 1;) {
        printf("The condition is always true, so this code is reachable!");
        printf("%f\n", i);
    }

    for(i = 0; false || false; i = i + 1;) {
        printf("The condition is always false, so this code is unreachable!");
        printf("%f\n", i);
    }

    for(i = 0; a && !false; i = i + 1;) {
        printf("The condition is indeterminate, so this code is reachable!");
        printf("%f\n", i);
    }

    return 0;
}