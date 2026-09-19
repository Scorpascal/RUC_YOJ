#include <stdio.h>

int main(void) {
    int A, B, n;
    if (scanf("%d %d %d", &A, &B, &n) != 3) return 0;
    A %= 7;
    B %= 7;
    if (n == 1 || n == 2) {
        printf("1\n");
        return 0;
    }
    int f1 = 1, f2 = 1, fn = 0;
    for (int i = 3; i <= n; ++i) {
        fn = (A * f2 + B * f1) % 7;
        f1 = f2;
        f2 = fn;
    }
    printf("%d\n", fn);
    return 0;
}