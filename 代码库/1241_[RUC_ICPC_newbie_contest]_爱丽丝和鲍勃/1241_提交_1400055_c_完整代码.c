#include <stdio.h>
#include <stdlib.h>

int main() {
    int T;
    if (scanf("%d", &T) != 1) return 0;
    while (T--) {
        int n;
        scanf("%d", &n);
        long long mx = -1;
        int cnt = 0;
        for (int i = 0; i < n; ++i) {
            long long x;
            scanf("%lld", &x);
            if (x > mx) {
                mx = x;
                cnt = 1;
            } else if (x == mx) {
                ++cnt;
            }
        }
        if (cnt % 2 == 1) printf("Alice\n");
        else printf("Bob\n");
    }
    return 0;
}