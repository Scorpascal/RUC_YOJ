#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    int64_t l, r;
} Interval;

// 高速输入：fread 缓冲解析
typedef struct {
    char *buf;
    size_t sz, pos, cap;
} FastIn;

static int fin_init(FastIn *fi) {
    const size_t CHUNK = 1 << 20; // 1MiB
    fi->buf = (char *)malloc(CHUNK);
    if (!fi->buf) return 0;
    fi->cap = CHUNK;
    fi->sz = 0;
    fi->pos = 0;

    for (;;) {
        if (fi->sz + CHUNK > fi->cap) {
            size_t ncap = fi->cap << 1;
            char *nbuf = (char *)realloc(fi->buf, ncap);
            if (!nbuf) { free(fi->buf); fi->buf = NULL; return 0; }
            fi->buf = nbuf;
            fi->cap = ncap;
        }
        size_t readn = fread(fi->buf + fi->sz, 1, CHUNK, stdin);
        fi->sz += readn;
        if (readn < CHUNK) break;
    }
    return 1;
}

static inline int fin_skip_ws(FastIn *fi) {
    while (fi->pos < fi->sz) {
        unsigned char c = (unsigned char)fi->buf[fi->pos];
        if (c > ' ') return 1;
        ++fi->pos;
    }
    return 0;
}

static inline int fin_read_int(FastIn *fi, int *out) {
    if (!fin_skip_ws(fi)) return 0;
    int sign = 1;
    char c = fi->buf[fi->pos];
    if (c == '-') { sign = -1; ++fi->pos; }
    int v = 0;
    while (fi->pos < fi->sz) {
        c = fi->buf[fi->pos];
        if ((unsigned char)c < '0' || (unsigned char)c > '9') break;
        v = v * 10 + (c - '0');
        ++fi->pos;
    }
    *out = v * sign;
    return 1;
}

static inline int fin_read_i64(FastIn *fi, int64_t *out) {
    if (!fin_skip_ws(fi)) return 0;
    int sign = 1;
    char c = fi->buf[fi->pos];
    if (c == '-') { sign = -1; ++fi->pos; }
    int64_t v = 0;
    while (fi->pos < fi->sz) {
        c = fi->buf[fi->pos];
        if ((unsigned char)c < '0' || (unsigned char)c > '9') break;
        v = v * 10 + (c - '0');
        ++fi->pos;
    }
    *out = sign == 1 ? v : -v;
    return 1;
}

// 内联比较：按 r 升序，r 相同按 l 升序
static inline int interval_less(const Interval *a, const Interval *b) {
    if (a->r != b->r) return a->r < b->r;
    return a->l < b->l;
}

// 迭代快速排序（三数取中 + 插入排序优化）
static void sort_intervals(Interval *arr, int n) {
    if (n <= 1) return;

    // 小段插入排序阈值
    const int TH = 32;

    // 辅助栈
    typedef struct { int lo, hi; } Range;
    Range *stack = (Range *)malloc(sizeof(Range) * 64); // 足够应付 1e6
    int top = 0;
    stack[top++] = (Range){0, n - 1};

    while (top) {
        Range rg = stack[--top];
        int lo = rg.lo, hi = rg.hi;
        while (hi - lo + 1 > TH) {
            int mid = (lo + hi) >> 1;
            // 三数取中
            Interval a = arr[lo], b = arr[mid], c = arr[hi];
            // 找中位值置于 mid
            if (interval_less(&b, &a)) { Interval t = a; a = b; b = t; }
            if (interval_less(&c, &b)) {
                Interval t = b; b = c; c = t;
                if (interval_less(&b, &a)) { Interval t2 = a; a = b; b = t2; }
            }
            arr[lo] = a; arr[mid] = b; arr[hi] = c;
            Interval pivot = arr[mid];

            int i = lo, j = hi;
            while (1) {
                while (interval_less(&arr[i], &pivot)) ++i;
                while (interval_less(&pivot, &arr[j])) --j;
                if (i <= j) {
                    Interval t = arr[i]; arr[i] = arr[j]; arr[j] = t;
                    ++i; --j;
                } else break;
            }

            // 处理较小段以减少栈深
            if (j - lo < hi - i) {
                if (i < hi) stack[top++] = (Range){i, hi};
                hi = j;
            } else {
                if (lo < j) stack[top++] = (Range){lo, j};
                lo = i;
            }
        }

        // 插入排序
        for (int k = lo + 1; k <= hi; ++k) {
            Interval key = arr[k];
            int p = k - 1;
            while (p >= lo && interval_less(&key, &arr[p])) {
                arr[p + 1] = arr[p];
                --p;
            }
            arr[p + 1] = key;
        }
    }
    free(stack);
}

int main(void) {
    FastIn fi;
    if (!fin_init(&fi)) { puts("0"); return 0; }

    int n = 0;
    if (!fin_read_int(&fi, &n) || n <= 0) { puts("0"); free(fi.buf); return 0; }

    Interval *arr = (Interval *)malloc(sizeof(Interval) * (size_t)n);
    if (!arr) { puts("0"); free(fi.buf); return 0; }

    int m = 0;
    for (; m < n; ++m) {
        int64_t l, r;
        if (!fin_read_i64(&fi, &l)) break;
        if (!fin_read_i64(&fi, &r)) break;
        if (l > r) { int64_t t = l; l = r; r = t; }
        arr[m].l = l;
        arr[m].r = r;
    }
    free(fi.buf);

    // 排序并贪心选择
    sort_intervals(arr, m);

    int64_t last_r = INT64_MIN;
    int count = 0;
    for (int i = 0; i < m; ++i) {
        if (arr[i].l >= last_r) {
            ++count;
            last_r = arr[i].r;
        }
    }

    printf("%d\n", count);
    free(arr);
    return 0;
}