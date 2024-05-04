int printf(string fmt, ...);

int main(bool a) {
    int i;

    if(1 || 1) {
        printf("The condition is always true, so this code is reachable!");
    }
    else {
        printf("The condition is always true, so this code is unreachable!");
    }

    if(0 && 0) {
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

    while(1 || 0) {
        printf("The condition is always true, so this code is reachable!");
    }

    while(1 && 0) {
        printf("The condition is always false, so this code is unreachable!");
    }

    while(a || 0) {
        printf("The condition is indeterminate, so this code is reachable!");
    }

    for(i = 0; 1 && 1; i = i + 1;) {
        printf("The condition is always true, so this code is reachable!");
    }

    for(i = 0; 0 || 0; i = i + 1;) {
        printf("The condition is always false, so this code is unreachable!");
    }

    for(i = 0; a && 1; i = i + 1;) {
        printf("The condition is indeterminate, so this code is reachable!");
    }

    return 0;
}