#include <stdio.h>
#include <stdbool.h>

#define MAXN 105
typedef struct { int x, y; } Node;

int main(void) {
    int n, k, x, y;
    if (scanf("%d %d", &n, &k) != 2) return 0;
    if (scanf("%d %d", &x, &y) != 2) return 0;

    static bool vis[MAXN][MAXN];
    static int dist[MAXN][MAXN];
    for (int i = 1; i <= n; ++i)
        for (int j = 1; j <= n; ++j)
            dist[i][j] = -1;

    // 8 个“骑士”移动
    const int dx[8] = {-1,-1, 1, 1,-2,-2, 2, 2};
    const int dy[8] = {-2, 2,-2, 2,-1, 1,-1, 1};

    // 简单数组队列
    Node q[MAXN*MAXN];
    int head = 0, tail = 0;

    vis[x][y] = true;
    dist[x][y] = 0;
    q[tail++] = (Node){x, y};

    int count = 1; // 包含起点

    while (head < tail) {
        Node cur = q[head++];
        int d = dist[cur.x][cur.y];
        if (d == k) continue; // 只需到第 k 层

        for (int i = 0; i < 8; ++i) {
            int nx = cur.x + dx[i];
            int ny = cur.y + dy[i];
            if (nx < 1 || nx > n || ny < 1 || ny > n) continue;
            if (vis[nx][ny]) continue;
            vis[nx][ny] = true;
            dist[nx][ny] = d + 1;
            ++count;
            q[tail++] = (Node){nx, ny};
        }
    }

    printf("%d\n", count);
    return 0;
}