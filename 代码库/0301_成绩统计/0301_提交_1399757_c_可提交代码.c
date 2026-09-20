#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int L, R;     // 区间 [L, R) 或 [L,100]
    int count;    // 人数
    int idx;      // 原始索引（用于稳定次序时参考下界）
} Bin;

int cmp_task1(const void *a, const void *b) {
    const Bin *x = (const Bin *)a;
    const Bin *y = (const Bin *)b;
    if (x->count != y->count) return y->count - x->count; // 人数降序
    // 人数相同，分数段比较低的先输出（即下界L升序）
    return x->L - y->L;
}

int main(void) {
    int n, m, g;
    if (scanf("%d %d %d", &n, &m, &g) != 3) return 0;
    int step = 100 / m;

    // 读成绩并统计
    int *counts = (int *)calloc(m, sizeof(int));
    if (!counts) return 0;

    for (int i = 0; i < n; ++i) {
        int s;
        scanf("%d", &s);
        int idx = (s == 100) ? (m - 1) : (s / step);
        if (idx < 0) idx = 0;
        if (idx >= m) idx = m - 1;
        counts[idx]++;
    }

    // 构建分段信息
    Bin *bins = (Bin *)malloc(sizeof(Bin) * m);
    if (!bins) { free(counts); return 0; }
    for (int i = 0; i < m; ++i) {
        bins[i].L = i * step;
        bins[i].R = (i == m - 1) ? 100 : (i + 1) * step;
        bins[i].count = counts[i];
        bins[i].idx = i;
    }

    // 任务1：排序输出（忽略人数为0的分段）
    if (g == 0 || g == 1) {
        Bin *copy = (Bin *)malloc(sizeof(Bin) * m);
        if (!copy) { free(bins); free(counts); return 0; }
        for (int i = 0; i < m; ++i) copy[i] = bins[i];

        qsort(copy, m, sizeof(Bin), cmp_task1);
        for (int i = 0; i < m; ++i) {
            if (copy[i].count == 0) continue;
            if (copy[i].R == 100) {
                printf("[%2d,%3d]: %d\n", copy[i].L, copy[i].R, copy[i].count);
            } else {
                printf("[%2d,%3d): %d\n", copy[i].L, copy[i].R, copy[i].count);
            }
        }
        free(copy);
    }

    // 任务2：直方图（全部分段，升序），必要时归一化到最多50个*
    if (g == 0) {
        printf("\n");
    }
    if (g == 0 || g == 2) {
        int maxc = 0;
        for (int i = 0; i < m; ++i) if (bins[i].count > maxc) maxc = bins[i].count;

        for (int i = 0; i < m; ++i) {
            int stars = bins[i].count;
            if (maxc > 50) {
                stars = (bins[i].count * 50) / maxc; // 下取整
            }
            if (bins[i].R == 100) {
                printf("[%2d,%3d]:", bins[i].L, bins[i].R);
            } else {
                printf("[%2d,%3d):", bins[i].L, bins[i].R);
            }
            for (int k = 0; k < stars; ++k) putchar('*');
            putchar('\n');
        }
    }

    free(bins);
    free(counts);
    return 0;
}