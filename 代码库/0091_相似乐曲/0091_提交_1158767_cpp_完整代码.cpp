#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define MAXN 100
#define BINS 16

typedef struct {
    int id;
    double dist;
} SongDist;

// 计算直方图
void get_hist(int len, int *freq, int *hist) {
    for (int i = 0; i < BINS; ++i) hist[i] = 0;
    for (int i = 0; i < len; ++i) {
        int bin = freq[i] / 16;
        hist[bin]++;
    }
}

// 计算欧氏距离
double calc_dist(int *h1, int *h2) {
    double sum = 0;
    for (int i = 0; i < BINS; ++i) {
        int d = h1[i] - h2[i];
        sum += d * d;
    }
    return sqrt(sum);
}

// 排序用比较函数
int cmp(const void *a, const void *b) {
    SongDist *sa = (SongDist*)a, *sb = (SongDist*)b;
    if (sa->dist < sb->dist) return -1;
    if (sa->dist > sb->dist) return 1;
    return sa->id - sb->id;
}

int main() {
    int n0, qfreq[MAXN], qhist[BINS];
    scanf("%d", &n0);
    for (int i = 0; i < n0; ++i) scanf("%d", &qfreq[i]);
    get_hist(n0, qfreq, qhist);

    int n, k;
    scanf("%d%d", &n, &k);

    SongDist arr[MAXN];
    int mfreq[MAXN], mhist[BINS];

    for (int i = 0; i < n; ++i) {
        int ni;
        scanf("%d", &ni);
        for (int j = 0; j < ni; ++j) scanf("%d", &mfreq[j]);
        get_hist(ni, mfreq, mhist);
        arr[i].id = i;
        arr[i].dist = calc_dist(qhist, mhist);
    }

    qsort(arr, n, sizeof(SongDist), cmp);

    for (int i = 0; i < k; ++i) {
        if (i) printf(" ");
        printf("%d", arr[i].id);
    }
    printf("\n");
    return 0;
}