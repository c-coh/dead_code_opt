int printf(string fmt, ...);

int main(bool a) {
    int i;

    if(true) {
        printf("The condition is always true, so this code is reachable!");
    }
    else {
        printf("The condition is always true, so this code is unreachable!");
    }

    if(false) {
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

    while(true || false) {
        printf("The condition is always true, so this code is reachable!");
    }

    while(true && false) {
        printf("The condition is always false, so this code is unreachable!");
    }

    while(a) {
        printf("The condition is indeterminate, so this code is reachable!");
    }

    for(i = 0; true; i = i + 1;) {
        printf("The condition is always true, so this code is reachable!");
    }

    for(i = 0; false; i = i + 1;) {
        printf("The condition is always false, so this code is unreachable!");
    }

    for(i = 0; a; i = i + 1;) {
        printf("The condition is indeterminate, so this code is reachable!");
    }
}