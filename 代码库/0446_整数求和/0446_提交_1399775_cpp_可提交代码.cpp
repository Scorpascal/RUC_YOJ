#include <stdio.h>

int main(void){
    int a, b, c;
    if (scanf("%d %d %d", &a, &b, &c) != 3) return 0;
    printf("%d\n", a + b + c);
    return 0;
}