#include <stdio.h>
#include <stdlib.h>

typedef unsigned long long u64;
typedef long long i64;

static i64 gcd_ll(i64 a, i64 b) {
    while (b) {
        i64 t = a % b;
        a = b;
        b = t;
    }
    return a >= 0 ? a : -a;
}

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    i64 *a = (i64*)malloc(sizeof(i64) * (size_t)n);
    for (int i = 0; i < n; ++i) {
        if (scanf("%lld", &a[i]) != 1) return 0;
    }

    // 每个元素为 (gcd_value, length)
    typedef struct { i64 g; int len; } Node;
    Node *cur = (Node*)malloc(sizeof(Node) * (size_t)n);
    Node *next = (Node*)malloc(sizeof(Node) * (size_t)n);
    int cur_sz = 0;

    u64 ans = 0;

    for (int i = 0; i < n; ++i) {
        int next_sz = 0;

        // 新起一个长度为 1 的子序列
        next[next_sz++] = (Node){ a[i], 1 };

        // 扩展之前所有以 i-1 结尾的子序列
        for (int j = 0; j < cur_sz; ++j) {
            i64 newg = gcd_ll(cur[j].g, a[i]);
            int newlen = cur[j].len + 1;

            if (next[next_sz - 1].g == newg) {
                // 合并相同 gcd，保留更大的长度
                if (newlen > next[next_sz - 1].len) {
                    next[next_sz - 1].len = newlen;
                }
            } else {
                next[next_sz++] = (Node){ newg, newlen };
            }
        }

        // 更新答案
        for (int j = 0; j < next_sz; ++j) {
            u64 val = (u64)next[j].len * (u64)(next[j].g >= 0 ? next[j].g : -next[j].g);
            if (val > ans) ans = val;
        }

        // 交换 cur/next
        Node *tmp = cur; cur = next; next = tmp;
        cur_sz = next_sz;
    }

    printf("%llu\n", ans);

    free(a);
    free(cur);
    free(next);
    return 0;
}