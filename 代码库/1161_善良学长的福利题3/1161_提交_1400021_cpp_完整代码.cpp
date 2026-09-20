#include <stdio.h>

int n, ans = 0;

// 用位运算优化
void dfs(int row, int col, int d1, int d2) {
    if (row == n) {
        ans++;
        return;
    }
    int available = ((1 << n) - 1) & ~(col | d1 | d2);
    while (available) {
        int p = available & -available; // 取最低位的1
        available -= p;
        dfs(row + 1, col | p, (d1 | p) << 1, (d2 | p) >> 1);
    }
}

int main() {
    scanf("%d", &n);
    dfs(0, 0, 0, 0);
    printf("%d\n", ans);
    return 0;
}