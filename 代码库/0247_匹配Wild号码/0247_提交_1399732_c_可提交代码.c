#include <stdio.h>
#include <string.h>
#include <ctype.h>

static void trim_newline(char *s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) {
        s[--n] = '\0';
    }
}

int main(void) {
    char X[64], W[64];
    while (1) {
        if (!fgets(X, sizeof(X), stdin)) return 0;
        trim_newline(X);
        // 跳过空行
        if (X[0] == '\0') continue;
        if (X[0] == '#') break;

        if (!fgets(W, sizeof(W), stdin)) return 0;
        trim_newline(W);

        size_t n = strlen(X);
        if (strlen(W) != n) {
            // 输入保证长度相等，若不等则输出0并继续
            printf("0\n");
            continue;
        }

        // DP: dp_eq 表示当前前缀与 W 相等的路径数，dp_gt 表示当前前缀已严格大于 W 的路径数
        unsigned long long dp_eq = 1ULL;
        unsigned long long dp_gt = 0ULL;

        for (size_t i = 0; i < n; ++i) {
            char xc = X[i];
            int wi = W[i] - '0';

            // 统计本位允许的数字集合
            int allowed_digits[10];
            int k = 0;
            if (xc == '?') {
                for (int d = 0; d <= 9; ++d) allowed_digits[k++] = d;
            } else if (isdigit((unsigned char)xc)) {
                allowed_digits[k++] = xc - '0';
            } else {
                // 非法字符，直接使结果为0
                k = 0;
            }

            unsigned long long next_eq = 0ULL;
            unsigned long long next_gt = 0ULL;

            // 从已大于的状态，任意允许的数字都保持已大于
            if (dp_gt > 0 && k > 0) {
                next_gt += dp_gt * (unsigned long long)k;
            }

            // 从相等前缀状态，比较与 W[i]
            if (dp_eq > 0) {
                for (int t = 0; t < k; ++t) {
                    int d = allowed_digits[t];
                    if (d < wi) {
                        // 小于则整数必小于，丢弃
                        continue;
                    } else if (d == wi) {
                        next_eq += dp_eq;
                    } else { // d > wi
                        next_gt += dp_eq;
                    }
                }
            }

            dp_eq = next_eq;
            dp_gt = next_gt;
        }

        printf("%llu\n", dp_gt);
    }
    return 0;
}