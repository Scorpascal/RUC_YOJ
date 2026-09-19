#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cmp_int(const void *a, const void *b) {
    int x = *(const int*)a, y = *(const int*)b;
    return (x<y) ? -1 : (x>y) ? 1 : 0;
}

// 二维树状数组
int *bit;
int bit_n, bit_m;

void bit_init(int n, int m) {
    bit_n = n; bit_m = m;
    bit = (int*)calloc((n+1) * (m+1), sizeof(int));
}

void bit_update(int x, int y, int delta) {
    for (int i = x; i <= bit_n; i += i & (-i))
        for (int j = y; j <= bit_m; j += j & (-j))
            bit[i * (bit_m+1) + j] += delta;
}

int bit_query(int x, int y) {
    int sum = 0;
    for (int i = x; i > 0; i -= i & (-i))
        for (int j = y; j > 0; j -= j & (-j))
            sum += bit[i * (bit_m+1) + j];
    return sum;
}

typedef struct { int val, x, y; } Node;
int cmp_node(const void *a, const void *b) {
    const Node *pa = (const Node*)a, *pb = (const Node*)b;
    if (pa->val != pb->val) return (pa->val < pb->val) ? -1 : 1;
    if (pa->x != pb->x) return (pa->x < pb->x) ? -1 : 1;
    return (pa->y < pb->y) ? -1 : 1;
}

int main() {
    int N, M;
    if (scanf("%d %d", &N, &M) != 2) return 0;

    int *is_expert = (int*)calloc(N*N, sizeof(int));
    for (int k = 0; k < M; ++k) {
        int r, c;
        if (scanf("%d %d", &r, &c) != 2) return 0;
        if (r>=1 && r<=N && c>=1 && c<=N) is_expert[(r-1)*N+(c-1)] = 1;
    }

    int *level_in = (int*)malloc(sizeof(int)*N*N);
    int max_level = 0;
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j) {
            scanf("%d", &level_in[i*N+j]);
            if (level_in[i*N+j] > max_level) max_level = level_in[i*N+j];
        }

    // 收集非专家 level 并升序
    int non_cnt = 0;
    for (int idx = 0; idx < N*N; ++idx) if (!is_expert[idx]) non_cnt++;
    int *non_levels = (int*)malloc(sizeof(int)*non_cnt);
    int p = 0;
    for (int idx = 0; idx < N*N; ++idx)
        if (!is_expert[idx]) non_levels[p++] = level_in[idx];
    qsort(non_levels, non_cnt, sizeof(int), cmp_int);

    // 构建最终矩阵
    int *final_mat = (int*)malloc(sizeof(int)*N*N);
    p = 0;
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            int idx = i*N + j;
            if (is_expert[idx]) {
                final_mat[idx] = level_in[idx];
            } else {
                final_mat[idx] = non_levels[p++];
            }
        }
    }

    // 正确计算不满意度：对每个非专家，统计左上(1..i,1..j)内 > 当前值 的个数
    Node *nodes = (Node*)malloc(sizeof(Node)*N*N);
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            int idx = i*N + j;
            nodes[idx].val = final_mat[idx];
            nodes[idx].x = i + 1; // 1-based
            nodes[idx].y = j + 1;
        }
    }
    qsort(nodes, N*N, sizeof(Node), cmp_node);

    bit_init(N, N);
    long long sum_unhappy = 0;

    for (int k = 0; k < N*N; ) {
        int v = nodes[k].val;
        int t = k;
        // 先把所有值等于 v 的位置加入 BIT，使 query 为 <= v 的数量
        while (t < N*N && nodes[t].val == v) {
            bit_update(nodes[t].x, nodes[t].y, 1);
            ++t;
        }
        // 计算每个位置左上子矩阵内 > v 的个数 = (x*y) - query(x,y)
        for (int m = k; m < t; ++m) {
            int i1 = nodes[m].x, j1 = nodes[m].y;
            int idx = (i1 - 1) * N + (j1 - 1);
            if (!is_expert[idx]) {
                int leq = bit_query(i1, j1);
                long long total = (long long)i1 * (long long)j1;
                sum_unhappy += (total - leq);
            }
        }
        k = t;
    }

    // 输出最终布局与不满意度
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (j) printf(" ");
            printf("%d", final_mat[i*N + j]);
        }
        printf("\n");
    }
    printf("%lld\n", sum_unhappy);

    free(bit);
    free(nodes);
    free(is_expert);
    free(level_in);
    free(non_levels);
    free(final_mat);
    return 0;
}//1595AK