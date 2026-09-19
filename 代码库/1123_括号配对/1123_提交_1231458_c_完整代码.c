#include <stdio.h>
#include <string.h>

#define MAXN 205
#define INF 1000000000

int dp[105][105];

int match(char a, char b) {
    return (a == '(' && b == ')') || (a == '[' && b == ']');
}

int main(void) {
    char s[MAXN];
    if (!fgets(s, sizeof(s), stdin)) return 0;
    size_t n = strcspn(s, "\r\n");
    s[n] = '\0';
    int len = (int)n;

    if (len == 0) {
        printf("0\n");
        return 0;
    }

    // 初始化：空段为 0，单字符为 1
    for (int i = 0; i < len; ++i) {
        dp[i][i] = 1;
    }

    for (int L = 2; L <= len; ++L) {
        for (int i = 0; i + L - 1 < len; ++i) {
            int j = i + L - 1;
            int best = INF;

            if (match(s[i], s[j])) {
                int inner = (i + 1 <= j - 1) ? dp[i + 1][j - 1] : 0;
                if (inner < best) best = inner;
            }

            for (int k = i; k < j; ++k) {
                int cand = dp[i][k] + dp[k + 1][j];
                if (cand < best) best = cand;
            }

            dp[i][j] = best == INF ? 0 : best;
        }
    }

    printf("%d\n", dp[0][len - 1]);
    return 0;
}