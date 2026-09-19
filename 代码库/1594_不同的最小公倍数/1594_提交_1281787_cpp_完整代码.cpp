#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(void) {
    const uint32_t MOD = 1000000007u;
    uint32_t n;
    if (scanf("%u", &n) != 1) return 0;
    if (n < 2) { // n=1 时只有空指数，答案为1
        printf("1\n");
        return 0;
    }

    // 埃氏筛
    uint32_t size = n + 1;
    unsigned char *is_comp = (unsigned char*)calloc(size, 1);
    if (!is_comp) return 0;

    uint32_t ans = 1;
    for (uint32_t i = 2; i <= n; ++i) {
        if (!is_comp[i]) {
            // i 是质数，计算最大 e 使得 i^e <= n
            uint64_t p = i;
            uint32_t e = 0;
            while (p <= n) {
                ++e;
                if (p > n / i) break; // 防止溢出同时提前终止
                p *= i;
            }
            ans = (uint64_t)ans * (e + 1) % MOD;

            // 标记合数
            if ((uint64_t)i * i <= n) {
                for (uint32_t j = i * i; j <= n; j += i) is_comp[j] = 1;
            }
        }
    }

    free(is_comp);
    printf("%u\n", ans);
    return 0;
}//1594AK