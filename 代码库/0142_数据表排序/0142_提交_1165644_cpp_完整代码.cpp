#include <stdio.h>
#include <stdlib.h>

#define MAXN 105

int table[MAXN][MAXN];
int sort_col[10];
int m, n, k;

// 比较函数
int cmp(const void *a, const void *b) {
    int *row1 = (int *)a;
    int *row2 = (int *)b;
    for (int i = 0; i < k; i++) {
        int col = sort_col[i] - 1; // 列号从1开始，数组下标从0
        if (row1[col] != row2[col])
            return row1[col] - row2[col];
    }
    // 按第1列作为最终比较
    return row1[0] - row2[0];
}

int main() {
    scanf("%d%d", &m, &n);
    scanf("%d", &k);
    for (int i = 0; i < k; i++) {
        scanf("%d", &sort_col[i]);
    }
    for (int i = 0; i < m; i++)
        for (int j = 0; j < n; j++)
            scanf("%d", &table[i][j]);

    qsort(table, m, sizeof(table[0]), cmp);

    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d", table[i][j]);
            if (j != n - 1) printf(" ");
        }
        printf("\n");
    }
    return 0;
}