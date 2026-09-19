#include <stdio.h>

long long gcd_ll(long long a, long long b) {
    while (b) { long long t = a % b; a = b; b = t; }
    return a;
}

long long pow10_int(int e) {
    long long r = 1;
    while (e--) r *= 10LL;
    return r;
}

int main(void) {
    long long a, b;
    int n, k, m;
    if (scanf("%lld %lld %d %d %d", &a, &b, &n, &k, &m) != 5) return 0;

    if (m == 0) { printf("0\n"); return 0; }
    if (k == 0) { printf("0\n"); return 0; }

    long long L1 = (m == 1) ? 1LL : pow10_int(m - 1);
    long long R1 = pow10_int(m) - 1;
    long long L = (a > L1) ? a : L1;
    long long R = (b < R1) ? b : R1;
    if (L > R) { printf("0\n"); return 0; }

    long long g = gcd_ll(10, k);
    if (n % g != 0) { printf("0\n"); return 0; }

    long long M = (10 / g) * (long long)k; // lcm(10, k)

    long long r = -1;
    for (int t = 0; t < k; ++t) {
        long long x = n + 10LL * t;
        if (x % k == 0) { r = (x % M + M) % M; break; }
    }
    if (r == -1) { printf("0\n"); return 0; }

    long long delta = (r - L) % M;
    if (delta < 0) delta += M;
    long long first = L + delta;
    if (first > R) { printf("0\n"); return 0; }

    long long cnt = (R - first) / M + 1;
    printf("%lld\n", cnt);
    return 0;
}