#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_base_to_int(const char *s, int r) {
    long long n = 0;
    for (const char *p = s; *p; ++p) {
        int d = *p - '0';
        n = n * r + d;
    }
    return (int)n;
}

int main(void) {
    // 为安全起见留足缓冲（题意下几十字符已足够）
    static char buf[1024];
    int T;
    if (scanf("%d", &T) != 1) return 0;
    while (T--) {
        int r;
        if (scanf("%d %1023s", &r, buf) != 2) return 0;
        int n = parse_base_to_int(buf, r);
        // 1/n，且 n >= 2
        int *first = (int*)malloc((size_t)n * sizeof(int));
        if (!first) return 0;
        for (int i = 0; i < n; ++i) first[i] = -1;

        // 结果小数位最多不会超过 n 位（余数空间大小）
        char *digits = (char*)malloc((size_t)(n + 5));
        if (!digits) { free(first); return 0; }

        int idx = 0;
        int cycle_start = -1;
        int rem = 1 % n; // 初始余数
        while (1) {
            if (rem == 0) {
                // 有限小数
                break;
            }
            if (first[rem] != -1) {
                cycle_start = first[rem];
                break;
            }
            first[rem] = idx;
            int t = rem * r;          // r <= 10, n <= 1e6，安全
            int dig = t / n;
            rem = t % n;
            digits[idx++] = (char)('0' + dig);
        }

        // 若有循环且循环节全为 r-1，则进一转为有限小数
        int integer_part = 0; // 对于 1/n 始终为 0；逻辑完整性保留
        if (cycle_start >= 0) {
            int all_max = 1;
            for (int i = cycle_start; i < idx; ++i) {
                if (digits[i] != '0' + (r - 1)) { all_max = 0; break; }
            }
            if (all_max) {
                int carry = 1;
                for (int i = cycle_start - 1; i >= 0 && carry; --i) {
                    int d = (digits[i] - '0') + carry;
                    if (d >= r) { digits[i] = '0'; carry = 1; }
                    else { digits[i] = (char)('0' + d); carry = 0; }
                }
                if (carry) {
                    // 进位到整数部分（对 1/n 不会发生，但为健壮性保留）
                    integer_part += 1;
                }
                // 去掉循环节，成为有限小数
                idx = cycle_start;
                cycle_start = -1;
            }
        }

        // 输出
        if (integer_part == 0) {
            putchar('0');
        } else {
            // 理论上不会发生（因 1/n < 1），但做兜底
            printf("%d", integer_part);
        }
        putchar('.');
        if (cycle_start < 0) {
            // 有限小数
            if (idx == 0) putchar('0'); // 例如 1/n 在该进制下恰好是 0.0... 的情况
            for (int i = 0; i < idx; ++i) putchar(digits[i]);
            putchar('\n');
        } else {
            // 循环小数
            for (int i = 0; i < cycle_start; ++i) putchar(digits[i]);
            putchar('[');
            for (int i = cycle_start; i < idx; ++i) putchar(digits[i]);
            putchar(']');
            putchar('\n');
        }

        free(first);
        free(digits);
    }
    return 0;
}