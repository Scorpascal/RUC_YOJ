// 题解：按截止时间排序，使用最大堆移除最长任务以满足截止时间

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int t;      // 加工所需时间
    int d;      // 截止时间
} Job;

static int cmp_by_deadline(const void* a, const void* b) {
    const Job* x = (const Job*)a;
    const Job* y = (const Job*)b;
    if (x->d != y->d) return (x->d < y->d) ? -1 : 1;
    // 可选：同截止时间时先处理短任务（对堆策略影响不大）
    if (x->t != y->t) return (x->t < y->t) ? -1 : 1;
    return 0;
}

/* 最大堆实现（存加工时长） */
static inline void heap_swap(int* a, int* b) {
    int t = *a; *a = *b; *b = t;
}
static void heap_push(int* heap, int* size, int val) {
    int i = ++(*size);
    heap[i] = val;
    while (i > 1) {
        int p = i >> 1;
        if (heap[p] >= heap[i]) break;
        heap_swap(&heap[p], &heap[i]);
        i = p;
    }
}
static int heap_pop(int* heap, int* size) {
    int ret = heap[1];
    int v = heap[(*size)--];
    if (*size == 0) return ret;
    heap[1] = v;
    int i = 1;
    while (1) {
        int l = i << 1, r = l + 1, m = i;
        if (l <= *size && heap[l] > heap[m]) m = l;
        if (r <= *size && heap[r] > heap[m]) m = r;
        if (m == i) break;
        heap_swap(&heap[i], &heap[m]);
        i = m;
    }
    return ret;
}

/* 快速输入（fread 解析），对大数据更稳 */
static const size_t BUF_SZ = 1 << 20;
static char ibuf[1 << 20];
static size_t ipos = 0, ilen = 0;

static inline void refill() {
    ilen = fread(ibuf, 1, BUF_SZ, stdin);
    ipos = 0;
}
static inline int nextChar() {
    if (ipos >= ilen) {
        refill();
        if (ilen == 0) return EOF;
    }
    return ibuf[ipos++];
}
static inline int readInt() {
    int c, s = 1, x = 0;
    do { c = nextChar(); if (c == EOF) return 0; } while (c <= ' ');
    if (c == '-') { s = -1; c = nextChar(); }
    for (; c >= '0' && c <= '9'; c = nextChar()) x = x * 10 + (c - '0');
    return x * s;
}

int main(void) {
    int n = readInt();
    if (n <= 0) { printf("0\n"); return 0; }

    Job* jobs = (Job*)malloc(sizeof(Job) * (size_t)n);
    if (!jobs) return 0;

    for (int i = 0; i < n; ++i) {
        int t = readInt();
        int d = readInt();
        jobs[i].t = t;
        jobs[i].d = d;
    }

    qsort(jobs, (size_t)n, sizeof(Job), cmp_by_deadline);

    // 最大堆数组，1-based
    int* heap = (int*)malloc(sizeof(int) * ((size_t)n + 2));
    if (!heap) { free(jobs); return 0; }
    int hsize = 0;
    long long time_sum = 0;

    for (int i = 0; i < n; ++i) {
        int t = jobs[i].t;
        int d = jobs[i].d;
        heap_push(heap, &hsize, t);
        time_sum += t;
        if (time_sum > d) {
            int longest = heap_pop(heap, &hsize);
            time_sum -= longest;
        }
    }

    printf("%d\n", hsize);

    free(heap);
    free(jobs);
    return 0;
}