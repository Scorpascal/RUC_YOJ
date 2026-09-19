#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

typedef struct {
    int x, y;
} Point;

static int cmp_x(const void *a, const void *b) {
    const Point *p = (const Point *)a;
    const Point *q = (const Point *)b;
    if (p->x != q->x) return (p->x < q->x) ? -1 : 1;
    if (p->y != q->y) return (p->y < q->y) ? -1 : 1;
    return 0;
}

static int cmp_y(const void *a, const void *b) {
    const Point *p = (const Point *)a;
    const Point *q = (const Point *)b;
    if (p->y != q->y) return (p->y < q->y) ? -1 : 1;
    if (p->x != q->x) return (p->x < q->x) ? -1 : 1;
    return 0;
}

static inline long long dist2(const Point *a, const Point *b) {
    long long dx = (long long)a->x - b->x;
    long long dy = (long long)a->y - b->y;
    return dx*dx + dy*dy;
}

/*
分治求最近点对平方距离。
参数：
- pts: 按 x 排序的点数组
- tmp: 辅助数组（用于按 y 合并），长度 >= r - l + 1
- l, r: 区间 [l, r]
返回：最近点对的平方距离
在返回时，pts[l..r] 将按 y 排序（用于上层合并）。
*/
static long long closest_pair(Point *pts, Point *tmp, int l, int r) {
    int n = r - l + 1;
    if (n <= 3) {
        long long ans = LLONG_MAX;
        for (int i = l; i <= r; ++i) {
            for (int j = i + 1; j <= r; ++j) {
                long long d = dist2(&pts[i], &pts[j]);
                if (d < ans) ans = d;
            }
        }
        // 将小段按 y 排序，便于上层合并
        qsort(pts + l, n, sizeof(Point), cmp_y);
        return ans;
    }

    int mid = (l + r) >> 1;
    int midx = pts[mid].x;

    long long dl = closest_pair(pts, tmp, l, mid);
    long long dr = closest_pair(pts, tmp, mid + 1, r);
    long long d = dl < dr ? dl : dr;

    // 归并：将左右段按 y 排序合并到 tmp，然后拷回 pts
    int i = l, j = mid + 1, k = 0;
    while (i <= mid && j <= r) {
        if (pts[i].y <= pts[j].y) tmp[k++] = pts[i++];
        else tmp[k++] = pts[j++];
    }
    while (i <= mid) tmp[k++] = pts[i++];
    while (j <= r) tmp[k++] = pts[j++];
    for (int t = 0; t < k; ++t) pts[l + t] = tmp[t];

    // 构建条带：|x - midx|^2 < d
    // 为避免 sqrt，比较 (dx*dx) < d
    int strip_size = 0;
    for (int t = l; t <= r; ++t) {
        long long dx = (long long)pts[t].x - midx;
        if (dx*dx < d) {
            tmp[strip_size++] = pts[t]; // tmp 作为条带数组，按 y 已排序
        }
    }

    // 在条带中，按 y 排序，最多检查前若干个点（理论上最多 7 个）
    for (int a = 0; a < strip_size; ++a) {
        // 向上检查，直到 (dy*dy) >= d 则可提前停止
        for (int b = a + 1; b < strip_size; ++b) {
            long long dy = (long long)tmp[b].y - tmp[a].y;
            if (dy*dy >= d) break;
            long long dd = dist2(&tmp[a], &tmp[b]);
            if (dd < d) d = dd;
        }
    }

    return d;
}

int main(void) {
    // 关闭缓冲提高 scanf/printf 性能（可选）
    // setvbuf(stdin, NULL, _IOFBF, 1<<20);
    // setvbuf(stdout, NULL, _IOFBF, 1<<20);

    int n;
    if (scanf("%d", &n) != 1) return 0;
    if (n < 2) {
        printf("0\n");
        return 0;
    }

    Point *pts = (Point *)malloc(sizeof(Point) * n);
    Point *tmp = (Point *)malloc(sizeof(Point) * n);
    if (!pts || !tmp) {
        fprintf(stderr, "memory allocation failed\n");
        return 1;
    }

    for (int i = 0; i < n; ++i) {
        scanf("%d %d", &pts[i].x, &pts[i].y);
    }

    // 按 x 排序（若 x 相等按 y 排序）
    qsort(pts, n, sizeof(Point), cmp_x);

    long long ans = closest_pair(pts, tmp, 0, n - 1);
    printf("%lld\n", ans);

    free(pts);
    free(tmp);
    return 0;
}