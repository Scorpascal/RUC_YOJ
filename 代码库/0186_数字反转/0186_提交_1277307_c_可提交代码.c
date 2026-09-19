#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(void) {
    long long n;
    if (scanf("%lld", &n) != 1) return 0;

    int negative = (n < 0);
    unsigned long long x = negative ? (unsigned long long)(-n) : (unsigned long long)n;
    unsigned long long rev = 0;

    do {
        rev = rev * 10 + (x % 10);
        x /= 10;
    } while (x > 0);

    if (negative) printf("-%llu\n", rev);
    else printf("%llu\n", rev);

    return 0;
}