#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void){
    int n,m;
    if (scanf("%d%d",&n,&m)!=2) return 0;
    int stride = m + 1;                // 每行留一个字节给 '\0'
    char *t = (char *)malloc((size_t)n * stride);
    for (int i = 0; i < n; ++i) {
        if (scanf("%s", t + (size_t)i * stride) != 1) return 0;
    }

    int total = n * m;
    int *dist = (int *)malloc((size_t)total * sizeof(int));
    const int INF = 1000000000;

    // 初始化
    for (int i = 0; i < n; ++i) {
        char *row = t + (size_t)i * stride;
        for (int j = 0; j < m; ++j) {
            dist[i*m + j] = (row[j] == '0') ? 0 : INF;
        }
    }

    // 左上 -> 右下
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            int idx = i*m + j;
            if (i > 0) {
                int v = dist[(i-1)*m + j] + 1;
                if (v < dist[idx]) dist[idx] = v;
            }
            if (j > 0) {
                int v = dist[i*m + j-1] + 1;
                if (v < dist[idx]) dist[idx] = v;
            }
        }
    }

    // 右下 -> 左上
    for (int i = n-1; i >= 0; --i) {
        for (int j = m-1; j >= 0; --j) {
            int idx = i*m + j;
            if (i+1 < n) {
                int v = dist[(i+1)*m + j] + 1;
                if (v < dist[idx]) dist[idx] = v;
            }
            if (j+1 < m) {
                int v = dist[i*m + j+1] + 1;
                if (v < dist[idx]) dist[idx] = v;
            }
        }
    }

    long long sum = 0;
    for (int i = 0; i < n; ++i) {
        char *row = t + (size_t)i * stride;
        for (int j = 0; j < m; ++j) {
            if (row[j] == '1') sum += dist[i*m + j];
        }
    }

    printf("%lld\n", sum);
    free(t);
    free(dist);
    return 0;
}