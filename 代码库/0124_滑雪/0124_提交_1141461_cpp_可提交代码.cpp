#include <stdio.h>
#define MAXN 55

int R, C;
int h[MAXN][MAXN];
int dp[MAXN][MAXN]; // 记忆化数组
int dx[4] = {0, 0, 1, -1};
int dy[4] = {1, -1, 0, 0};

int max(int a, int b) { return a > b ? a : b; }

// 记忆化DFS，返回从(x, y)出发的最长滑坡长度
int dfs(int x, int y) {
    if (dp[x][y]) return dp[x][y];
    int res = 1;
    for (int d = 0; d < 4; d++) {
        int nx = x + dx[d], ny = y + dy[d];
        if (nx >= 1 && nx <= R && ny >= 1 && ny <= C && h[nx][ny] < h[x][y]) {
            res = max(res, dfs(nx, ny) + 1);
        }
    }
    dp[x][y] = res;
    return res;
}

int main() {
    scanf("%d%d", &R, &C);
    for (int i = 1; i <= R; i++)
        for (int j = 1; j <= C; j++)
            scanf("%d", &h[i][j]);
    int ans = 0;
    for (int i = 1; i <= R; i++)
        for (int j = 1; j <= C; j++)
            ans = max(ans, dfs(i, j));
    printf("%d\n", ans);
    return 0;
}