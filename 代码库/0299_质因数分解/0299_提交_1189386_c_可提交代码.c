#include <stdio.h>

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    for (int p = 2; p * p <= n; ++p) {
        if (n % p == 0) {
            int cnt = 0;
            while (n % p == 0) {
                n /= p;
                ++cnt;
            }
            printf("%d:%d\n", p, cnt);
        }
    }
    if (n > 1) printf("%d:%d\n", n, 1);
    return 0;
}