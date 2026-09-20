#include <stdio.h>
int main() {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    long long ans = 0;
    long long prefix = 0;
    for (int i = 1; i <= n; ++i) {
        long long x;
        scanf("%lld", &x);
        prefix += x;
        if (i >= 2 && prefix > 0) ans += prefix;
    }
    printf("%lld\n", ans);
    return 0;
}