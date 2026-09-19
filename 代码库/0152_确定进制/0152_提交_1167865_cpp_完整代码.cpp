#include <stdio.h>
#include <string.h>

static long long convert(const char *s, int base) {
    long long value = 0;
    for (size_t i = 0; s[i]; ++i) {
        int digit = s[i] - '0';
        if (digit >= base) return -1;
        value = value * base + digit;
    }
    return value;
}

int main(void) {
    int T;
    if (scanf("%d", &T) != 1) return 0;

    for (int case_no = 1; case_no <= T; ++case_no) {
        char p[32], q[32], r[32];
        if (scanf("%31s %31s %31s", p, q, r) != 3) return 0;

        int max_digit = 0;
        const char *arr[] = {p, q, r};
        for (int i = 0; i < 3; ++i) {
            for (size_t j = 0; arr[i][j]; ++j) {
                int digit = arr[i][j] - '0';
                if (digit > max_digit) max_digit = digit;
            }
        }

        int start_base = max_digit + 1;
        if (start_base < 2) start_base = 2;

        int answer = 0;
        for (int base = start_base; base <= 16; ++base) {
            long long vp = convert(p, base);
            long long vq = convert(q, base);
            long long vr = convert(r, base);
            if (vp < 0 || vq < 0 || vr < 0) continue;

            __int128 prod = (__int128)vp * (__int128)vq;
            if (prod == (__int128)vr) {
                answer = base;
                break;
            }
        }

        printf("%d\n", answer);
    }
    return 0;
}