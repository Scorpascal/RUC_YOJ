#include <stdio.h>

int main(void) {
    printf("%d\n", 0xFFF);                         // 4095
    printf("%d\n", 017777);                        // 8191
    printf("%lld\n", 2147483647LL + 2147483647LL); // 4294967294
    printf("%llu\n", 9223372036854775807ULL + 9223372036854775807ULL); // 18446744073709551614
    return 0;
}