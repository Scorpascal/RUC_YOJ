#include <stdio.h>

int main(void) {
    int d;
    if (scanf("%d", &d) != 1) return 0;

    int a = d / 1000;
    int b = d / 100 % 10;
    int c = d / 10 % 10;
    int e = d % 10;

    a = (a + 5) % 10;
    b = (b + 5) % 10;
    c = (c + 5) % 10;
    e = (e + 5) % 10;

    int res = e * 1000 + c * 100 + b * 10 + a;
    printf("%d\n", res);
    return 0;
}