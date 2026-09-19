#include <stdio.h>
#include <stdint.h>

static void print_u128(unsigned __int128 x) {
    if (x == 0) { putchar('0'); return; }
    char buf[48];
    int i = 0;
    while (x > 0) {
        buf[i++] = '0' + (int)(x % 10);
        x /= 10;
    }
    while (i--) putchar(buf[i]);
}

// 将长度 a 连续向上二分 s 次后的结果：ceil(a / 2^s)
static inline long long ceil_div_pow2_ll(long long a, int s) {
    if (s <= 0) return a;
    if (a <= 1) return a ? 1 : 0;
    if (s >= 62) return 1; // 足够多次后必为 1
    return (a + ((1LL << s) - 1)) >> s;
}

// 把长度 a 变为 1 至少需要的切刀数（每次向上二分）
static int cuts_to_one(long long a) {
    int c = 0;
    while (a > 1) {
        a = (a + 1) / 2;
        ++c;
    }
    return c;
}

int main() {
    int T;
    if (scanf("%d", &T) != 1) return 0;
    while (T--) {
        long long n, m, k;
        if (scanf("%lld%lld%lld", &n, &m, &k) != 3) break;

        int lgn = cuts_to_one(n);
        int lgm = cuts_to_one(m);

        unsigned __int128 min_rem = (unsigned __int128)n * (unsigned __int128)m;

        if (k >= (long long)lgn + (long long)lgm) {
            // 两边都能切到 1，剩余面积为 1
            min_rem = 1;
        } else {
            long long lo = k - lgm; if (lo < 0) lo = 0;
            long long hi = k; if (hi > lgn) hi = lgn;
            for (long long x = lo; x <= hi; ++x) {
                // x 次切在 n 方向，k-x 次切在 m 方向
                long long a = ceil_div_pow2_ll(n, (int)x);
                long long b = ceil_div_pow2_ll(m, (int)(k - x));
                unsigned __int128 rem = (unsigned __int128)a * (unsigned __int128)b;
                if (rem < min_rem) min_rem = rem;
            }
        }

        unsigned __int128 total = (unsigned __int128)n * (unsigned __int128)m;
        unsigned __int128 ans = total - min_rem;
        print_u128(ans);
        putchar('\n');
    }
    return 0;
}