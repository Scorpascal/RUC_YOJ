#include <stdio.h>
#include <string.h>
#include <limits.h>

long long val(int l, int r, long long prefix[], long long pow10[]) {
    return prefix[r] - prefix[l-1] * pow10[r-l+1];
}

int main() {
    char s[105];
    int k;
    if (scanf("%s %d", s, &k) != 2) return 0;
    int L = strlen(s);
    int parts = k + 1;
    // prefix and pow10, 1-based
    long long prefix[105];
    long long pow10[105];
    prefix[0] = 0;
    pow10[0] = 1;
    for (int i = 1; i <= L; ++i) {
        prefix[i] = prefix[i-1] * 10 + (s[i-1] - '0');
        pow10[i] = pow10[i-1] * 10;
    }
    // dp[i][t]: max product using first i digits divided into t parts
    static long long dp[105][105];
    for (int i = 0; i <= L; ++i)
        for (int t = 0; t <= parts; ++t)
            dp[i][t] = 0;
    // base: one part
    for (int i = 1; i <= L; ++i) dp[i][1] = val(1, i, prefix, pow10);
    for (int t = 2; t <= parts; ++t) {
        for (int i = t; i <= L; ++i) {
            long long best = 0;
            for (int p = t-1; p <= i-1; ++p) {
                long long left = dp[p][t-1];
                if (left == 0) continue;
                long long right = val(p+1, i, prefix, pow10);
                long long prod = left * right;
                if (prod > best) best = prod;
            }
            dp[i][t] = best;
        }
    }
    printf("%lld\n", dp[L][parts]);
    return 0;
}