#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXN 25
typedef unsigned int U32;

static int n, m;
static U32 adj[MAXN]; // 原图邻接（不安全边）
static int best_size = 0;

static int result_cap = 0;
static int result_cnt = 0;
static int *res_data = NULL; // result_cnt * n
static int *res_sizes = NULL;

// 将当前独立集保存为升序
static void save_solution(int *set, int sz) {
    if (sz > best_size) {
        best_size = sz;
        result_cnt = 0;
    }
    if (sz == best_size) {
        if (result_cnt == result_cap) {
            result_cap = result_cap ? result_cap * 2 : 64;
            res_data = (int *)realloc(res_data, result_cap * n * sizeof(int));
            res_sizes = (int *)realloc(res_sizes, result_cap * sizeof(int));
        }
        // 升序排序
        for (int i = 1; i < sz; ++i) {
            int key = set[i], j = i - 1;
            while (j >= 0 && set[j] > key) { set[j + 1] = set[j]; j--; }
            set[j + 1] = key;
        }
        memcpy(res_data + result_cnt * n, set, sz * sizeof(int));
        res_sizes[result_cnt] = sz;
        result_cnt++;
    }
}

// 回溯枚举最大独立集：curr 为已选集合（存 1-based 编号），candidates 为可选顶点集合(bitset)
static void dfs(U32 candidates, int *curr, int sz) {
    if (!candidates) {
        save_solution(curr, sz);
        return;
    }
    // 简单上界剪枝：剩余候选数不足以超过当前最优
    int rem = __builtin_popcount(candidates);
    if (sz + rem < best_size) return;

    // 选择一个顶点 v（最小编号优先，利于生成升序）
    while (candidates) {
        int v = __builtin_ctz(candidates); // 0-based
        candidates &= candidates - 1;      // 移除 v

        // 检查 v 与当前集合是否冲突（原图边冲突）
        int conflict = 0;
        for (int i = 0; i < sz; ++i) {
            int u = curr[i] - 1;
            if (adj[v] & (1u << u)) { conflict = 1; break; }
        }
        if (conflict) continue;

        // 选 v
        curr[sz] = v + 1;
        // 新候选：必须与 v 不相邻，且保持不与已选的任何顶点相邻
        U32 newCand = candidates;
        // 去掉与 v 相邻的点
        newCand &= ~adj[v];
        // 保持与已选点不相邻（由于我们每次都检查冲突，这里已经满足；可不再额外过滤）

        dfs(newCand, curr, sz + 1);
        // 不选 v 的分支隐含于继续循环
    }

    // 到这里，所有选择已尝试，若 sz 达到 best_size，save_solution 在进入时已处理
}

static int lex_cmp(const int *a, int sa, const int *b, int sb) {
    int i = 0;
    while (i < sa && i < sb) {
        if (a[i] != b[i]) return a[i] - b[i];
        i++;
    }
    return sa - sb;
}

static int cmp_res(const void *pa, const void *pb) {
    const int *a = *(const int **)pa;
    const int *b = *(const int **)pb;
    int sa = a[-1];
    int sb = b[-1];
    return lex_cmp(a, sa, b, sb);
}

int main(void) {
    if (scanf("%d %d", &n, &m) != 2) return 0;
    if (n <= 0 || n > MAXN) return 0;

    for (int i = 0; i < n; ++i) adj[i] = 0;

    for (int i = 0; i < m; ++i) {
        int a, b;
        if (scanf("%d %d", &a, &b) != 2) return 0;
        if (a < 1 || a > n || b < 1 || b > n || a == b) continue;
        a--; b--;
        adj[a] |= (1u << b);
        adj[b] |= (1u << a);
    }

    U32 all = (n == 32) ? 0xFFFFFFFFu : ((1u << n) - 1);
    int curr[MAXN];
    dfs(all, curr, 0);

    if (result_cnt == 0) return 0;

    // 排序输出
    int **ptrs = (int **)malloc(result_cnt * sizeof(int *));
    for (int i = 0; i < result_cnt; ++i) {
        int sz = res_sizes[i];
        int *arr = (int *)malloc((sz + 1) * sizeof(int));
        arr[0] = sz;
        memcpy(arr + 1, res_data + i * n, sz * sizeof(int));
        ptrs[i] = arr + 1;
    }
    qsort(ptrs, result_cnt, sizeof(int *), cmp_res);

    for (int i = 0; i < result_cnt; ++i) {
        int sz = ptrs[i][-1];
        for (int j = 0; j < sz; ++j) {
            if (j) putchar(' ');
            printf("%d", ptrs[i][j]);
        }
        putchar('\n');
    }

    for (int i = 0; i < result_cnt; ++i) free(ptrs[i] - 1);
    free(ptrs);
    free(res_data);
    free(res_sizes);
    return 0;
}//1598AK