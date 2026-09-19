#include <stdio.h>

int main() {
    int n, k;
    if (scanf("%d %d", &n, &k) != 2) return 0;

    long long score1 = 0, score2 = 0;
    int streak1 = 0, streak2 = 0;

    for (int i = 0; i < n; ++i) {
        long long a, b, c, d, e;
        long long x;
        long long ans1, ans2;

        if (scanf("%lld %lld %lld %lld %lld %lld %lld %lld",
                  &a, &b, &c, &d, &e, &x, &ans1, &ans2) != 8) return 0;

        // Horner 法计算 ax^4 + bx^3 + cx^2 + dx + e
        long long val = (((a * x + b) * x + c) * x + d) * x + e;

        // 蓬蓬
        if (ans1 == val) {
            streak1++;
            long long extra = (streak1 > k) ? (streak1 - k) : 0;
            score1 += 1 + extra;
        } else {
            streak1 = 0;
        }

        // 凯凯
        if (ans2 == val) {
            streak2++;
            long long extra = (streak2 > k) ? (streak2 - k) : 0;
            score2 += 1 + extra;
        } else {
            streak2 = 0;
        }
    }

    printf("%lld %lld\n", score1, score2);
    return 0;
}