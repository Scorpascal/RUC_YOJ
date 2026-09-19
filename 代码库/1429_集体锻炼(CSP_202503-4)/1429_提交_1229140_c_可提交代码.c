#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static const uint64_t MOD = 998244353ULL;

static inline uint64_t gcd_u64(uint64_t a, uint64_t b){
    while (b){ uint64_t t = a % b; a = b; b = t; }
    return a;
}
static inline uint64_t add_mod(uint64_t a, uint64_t b){
    a += b; if (a >= MOD) a -= MOD; return a;
}
static inline uint64_t sub_mod(uint64_t a, uint64_t b){
    return (a >= b) ? (a - b) : (a + MOD - b);
}
static inline uint64_t mul_mod(uint64_t a, uint64_t b){
    return (uint64_t)((__int128)a * b % MOD);
}

typedef struct {
    uint64_t g;       // 当前段的 gcd
    uint64_t cnt;     // 子数组数量
    uint64_t sumLen;  // 子数组长度之和
} Node;

int main(void){
    int n;
    if (scanf("%d", &n) != 1) return 0;
    uint64_t* a = (uint64_t*)malloc(sizeof(uint64_t) * n);
    for (int i = 0; i < n; ++i) scanf("%llu", &a[i]);

    // gcd 段数量为 O(log A)，A<=1e6，这里 64 足够
    Node prev[64], cur[64];
    int prev_sz = 0, cur_sz = 0;

    uint64_t ans = 0;

    for (int i = 0; i < n; ++i){
        cur_sz = 0;

        // 1) 以 a[i] 单独开始的段：len=1
        cur[cur_sz++] = (Node){ a[i], 1, 1 };

        // 2) 扩展上一轮的所有段（长度都 +1）
        for (int j = 0; j < prev_sz; ++j){
            uint64_t ng = gcd_u64(prev[j].g, a[i]);
            uint64_t ncnt = prev[j].cnt;
            uint64_t nsumLen = prev[j].sumLen + prev[j].cnt; // 每个子数组长度+1

            if (cur_sz > 0 && cur[cur_sz - 1].g == ng){
                cur[cur_sz - 1].cnt    += ncnt;
                cur[cur_sz - 1].sumLen += nsumLen;
            }else{
                cur[cur_sz++] = (Node){ ng, ncnt, nsumLen };
            }
        }

        // 3) 统计以 r=i+1 为右端点的贡献
        uint64_t R = (uint64_t)(i + 1);
        uint64_t Rmod = R % MOD;
        uint64_t Rp1mod = (R + 1) % MOD;

        for (int j = 0; j < cur_sz; ++j){
            uint64_t gmod = cur[j].g % MOD;

            // sum_l = cnt*(R+1) - sumLen
            uint64_t cnt_mod = cur[j].cnt % MOD;
            uint64_t sumLen_mod = cur[j].sumLen % MOD;
            uint64_t sum_l_mod = sub_mod(mul_mod(cnt_mod, Rp1mod), sumLen_mod);

            uint64_t contrib = mul_mod(mul_mod(gmod, Rmod), sum_l_mod);
            ans = add_mod(ans, contrib);
        }

        // 4) 滚动
        prev_sz = cur_sz;
        for (int j = 0; j < cur_sz; ++j) prev[j] = cur[j];
    }

    printf("%llu\n", ans % MOD);
    free(a);
    return 0;
}