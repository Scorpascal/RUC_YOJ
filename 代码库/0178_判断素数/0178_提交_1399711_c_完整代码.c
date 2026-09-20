#include <stdio.h>
#include <stdbool.h>

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 0;

    const int MAXA = 1000;
    bool is_prime[MAXA + 1];
    for (int i = 0; i <= MAXA; ++i) is_prime[i] = true;
    is_prime[0] = is_prime[1] = false;
    for (int p = 2; p * p <= MAXA; ++p) {
        if (is_prime[p]) {
            for (int q = p * p; q <= MAXA; q += p) is_prime[q] = false;
        }
    }

    for (int i = 0; i < n; ++i) {
        int a;
        if (scanf("%d", &a) != 1) a = 0;
        printf("%s", (a >= 0 && a <= MAXA && is_prime[a]) ? "YES" : "NO");
        if (i + 1 < n) putchar(' ');
    }
    putchar('\n');
    return 0;
}