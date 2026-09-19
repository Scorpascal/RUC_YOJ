#include <stdio.h>

int is_prime(int x) {
    if (x < 2) return 0;
    if (x % 2 == 0) return x == 2;
    for (int i = 3; i * i <= x; i += 2) {
        if (x % i == 0) return 0;
    }
    return 1;
}

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 0;

    for (int a = 3; a <= n / 2; a += 2) {
        int b = n - a;
        if (is_prime(a) && is_prime(b)) {
            printf("%d=%d+%d\n", n, a, b);
            return 0;
        }
    }
    return 0;
}