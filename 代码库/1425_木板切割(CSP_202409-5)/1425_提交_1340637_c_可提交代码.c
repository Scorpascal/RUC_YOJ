#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXN 100000
#define MAXK 100000
#define BITSET_LIMIT 4096  // m<=此阈值时启用位集聚合

// Treap 节点（每个全局段 i 对应一个固定节点）
static int lc[MAXN + 5], rc[MAXN + 5], pri[MAXN + 5];
static int key_[MAXN + 5], col_[MAXN + 5];

// 维护颜色段数聚合
static int fstC[MAXN + 5], lstC[MAXN + 5], segCnt[MAXN + 5];

// 可选：位集聚合不同颜色（当 m 较小）
static int BWORDS = 0;                 // 位集 64bit word 数；=0 表示关闭位集
static unsigned long long *bits = NULL; // (n+1)*BWORDS 大小

// 每块木板的一棵 Treap（按 key=全局段编号 有序）
static int root[MAXK + 5];
static int n, m, k;

// 颜色统计（大 m 时备用：时间戳避免每次清零）
static int *vis, curTag = 1;

// 快速随机数（Treap 优先级）
static inline unsigned rng() {
    static unsigned x = 123456789u;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return x;
}

static inline unsigned long long* BS(int u) { return bits + (size_t)u * BWORDS; }

static inline void pull(int u) {
    // 聚合颜色段数
    fstC[u] = lc[u] ? fstC[lc[u]] : col_[u];
    lstC[u] = rc[u] ? lstC[rc[u]] : col_[u];
    int seg = 1;
    if (lc[u]) { seg += segCnt[lc[u]]; if (lstC[lc[u]] == col_[u]) --seg; }
    if (rc[u]) { seg += segCnt[rc[u]]; if (col_[u] == fstC[rc[u]]) --seg; }
    segCnt[u] = seg;

    // 聚合不同颜色的位集
    if (BWORDS) {
        unsigned long long *bu = BS(u);
        if (lc[u]) {
            unsigned long long *bl = BS(lc[u]);
            for (int i = 0; i < BWORDS; ++i) bu[i] = bl[i];
        } else {
            for (int i = 0; i < BWORDS; ++i) bu[i] = 0ULL;
        }
        if (rc[u]) {
            unsigned long long *br = BS(rc[u]);
            for (int i = 0; i < BWORDS; ++i) bu[i] |= br[i];
        }
        int idx = (col_[u] - 1) >> 6;
        int off = (col_[u] - 1) & 63;
        bu[idx] |= (1ULL << off);
    }
}

static inline int merge_(int a, int b) {
    if (!a) return b;
    if (!b) return a;
    if (pri[a] < pri[b]) {
        rc[a] = merge_(rc[a], b);
        pull(a);
        return a;
    } else {
        lc[b] = merge_(a, lc[b]);
        pull(b);
        return b;
    }
}

// 按 key < k，把 root 分成 (A, B)
static inline void split_less(int root0, int kkey, int *A, int *B) {
    if (!root0) { *A = *B = 0; return; }
    if (key_[root0] < kkey) {
        int tA, tB;
        split_less(rc[root0], kkey, &tA, &tB);
        rc[root0] = tA;
        pull(root0);
        *A = root0; *B = tB;
    } else {
        int tA, tB;
        split_less(lc[root0], kkey, &tA, &tB);
        lc[root0] = tB;
        pull(root0);
        *A = tA; *B = root0;
    }
}

// 备用：在 mid 上中序遍历统计“不同颜色数”（仅在关闭位集时使用）
static void count_distinct_inorder(int u, int *diff_cnt) {
    if (!u) return;
    count_distinct_inorder(lc[u], diff_cnt);
    int c = col_[u];
    if (vis[c] != curTag) { vis[c] = curTag; ++(*diff_cnt); }
    count_distinct_inorder(rc[u], diff_cnt);
}

// 快速 IO
static inline int read_int() {
    int c = getchar_unlocked(), x = 0;
    while (c <= 32) c = getchar_unlocked();
    for (; c > 32; c = getchar_unlocked()) x = x * 10 + (c - '0');
    return x;
}
static inline void write_int(int x) {
    char s[12]; int n = 0;
    if (x == 0) { putchar_unlocked('0'); return; }
    while (x) { s[n++] = (char)('0' + (x % 10)); x /= 10; }
    while (n--) putchar_unlocked(s[n]);
}

int main() {
    n = read_int(); m = read_int(); k = read_int();

    // 位集策略选择
    if (m <= BITSET_LIMIT) BWORDS = (m + 63) >> 6; else BWORDS = 0;
    if (BWORDS) {
        bits = (unsigned long long*)calloc((size_t)(n + 5) * BWORDS, sizeof(unsigned long long));
    }
    vis = (int*)calloc(m + 2, sizeof(int));

    for (int i = 1; i <= n; ++i) {
        int c = read_int();
        key_[i] = i;
        col_[i] = c;
        lc[i] = rc[i] = 0;
        pri[i] = (int)rng();

        fstC[i] = lstC[i] = c;
        segCnt[i] = 1;
        if (BWORDS) {
            int idx = (c - 1) >> 6, off = (c - 1) & 63;
            BS(i)[idx] |= (1ULL << off);
        }
    }

    // 初始：所有段都在 1 号木板（按 key 升序构建 Treap）
    int r = 0;
    for (int i = 1; i <= n; ++i) r = merge_(r, i);
    root[1] = r;
    int board_cnt = 1;

    for (int op = 0; op < k; ++op) {
        int x = read_int(), l = read_int(), rr = read_int();

        // 对 x 号木板：按 <l, [l,rr], >rr 三段 split
        int A, B, mid, C;
        split_less(root[x], l, &A, &B);        // B: key >= l
        split_less(B, rr + 1, &mid, &C);       // mid: [l, rr]
        root[x] = merge_(A, C);                // 去掉 mid 后，剩余合并回 x

        int newId = ++board_cnt;               // 新木板编号
        root[newId] = mid;                     // 切下来的部分直接挂到新板

        if (!mid) {
            putchar_unlocked('0'); putchar_unlocked(' ');
            putchar_unlocked('0'); putchar_unlocked('\n');
            continue;
        }

        int seg = segCnt[mid];
        int diff = 0;
        if (BWORDS) {
            unsigned long long *bm = BS(mid);
            for (int i = 0; i < BWORDS; ++i) diff += __builtin_popcountll(bm[i]);
        } else {
            ++curTag;
            count_distinct_inorder(mid, &diff);
        }
        write_int(diff); putchar_unlocked(' ');
        write_int(seg);  putchar_unlocked('\n');
    }

    free(vis);
    if (bits) free(bits);
    return 0;
}