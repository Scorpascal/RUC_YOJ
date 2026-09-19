#include <stdio.h>
#include <stdint.h>

#define LIMIT 100000  // sqrt(1e10)

static int primes[LIMIT];
static char is_comp[LIMIT + 1];
static int pcnt = 0;

static void sieve(void) {
    for (int i = 2; i <= LIMIT; ++i) {
        if (!is_comp[i]) {
            primes[pcnt++] = i;
            if ((long long)i * i <= LIMIT) {
                for (long long j = (long long)i * i; j <= LIMIT; j += i)
                    is_comp[j] = 1;
            }
        }
    }
}

int main(void) {
    sieve();

    int q;
    if (scanf("%d", &q) != 1) return 0;

    while (q--) {
        unsigned long long n, k;
        if (scanf("%llu %llu", &n, &k) != 2) break;

        unsigned long long x = n;
        unsigned long long ans = 1;

        for (int i = 0; i < pcnt && (unsigned long long)primes[i] * primes[i] <= x; ++i) {
            int p = primes[i];
            if (x % p == 0) {
                unsigned long long cnt = 0, powp = 1;
                do {
                    x /= p;
                    cnt++;
                    powp *= (unsigned long long)p;
                } while (x % p == 0);
                if (cnt >= k) ans *= powp;  // 保留 t_i >= k
            }
        }
        if (x > 1) { // 剩余一个大质因子（指数为 1）
            if (1ULL >= k) ans *= x;
        }

        // 若全部删除则 ans 仍为 1，符合题意
        printf("%llu\n", ans);
    }
    return 0;
}