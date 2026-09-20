#include <stdio.h>
#include <string.h>

#ifdef __APPLE__
#define getchar_fast getchar_unlocked
#else
#define getchar_fast getchar
#endif

static inline int read_int() {
    int c = getchar_fast(), x = 0, neg = 0;
    while (c!='-' && (c<'0' || c>'9')) c = getchar_fast();
    if (c=='-') { neg = 1; c = getchar_fast(); }
    while (c>='0' && c<='9') { x = x*10 + (c-'0'); c = getchar_fast(); }
    return neg ? -x : x;
}

const int INF = 0x3f3f3f3f;
static int a[100005+5], k[100005+5];
static int dist_[100005+5];
static int parent_[100005+5];   // DSU: next unprocessed position
static int q[100005+5];

static int n;

static int dsu_find(int x) {
    if (x > n) return n + 1;
    if (parent_[x] == x) return x;
    return parent_[x] = dsu_find(parent_[x]);
}

int main() {
    n = read_int();
    for (int i = 1; i <= n; ++i) a[i] = read_int();
    for (int i = 1; i <= n; ++i) k[i] = read_int();

    if (n == 1) { printf("0\n"); return 0; }

    for (int i = 1; i <= n+1; ++i) parent_[i] = i;  // n+1 作为哨兵
    memset(dist_, -1, sizeof(int)*(n+2));

    int hh = 0, tt = 0;
    dist_[1] = 0;
    q[tt++] = 1;

    while (hh < tt) {
        int i = q[hh++];

        long long rll = (long long)i + (long long)k[i];
        int r = (rll > n) ? n : (int)rll;

        int j = dsu_find(i + 1);
        while (j <= r) {
            int v = j - a[j];          // 退 a[j] 步后的站立位置
            if (dist_[v] == -1) {
                dist_[v] = dist_[i] + 1;
                if (v == n) {           // 及早结束
                    printf("%d\n", dist_[v]);
                    return 0;
                }
                q[tt++] = v;
            }
            // 标记 j 已处理，合并到 j+1，并跳到下一个未处理落点
            parent_[j] = dsu_find(j + 1);
            j = dsu_find(j);
        }
    }

    printf("-1\n");
    return 0;
}