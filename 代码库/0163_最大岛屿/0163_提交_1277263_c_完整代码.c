#include <stdio.h>
#include <stdbool.h>

#define MAXN 105

int n, m;
char grid[MAXN][MAXN];
bool vis[MAXN][MAXN];

int dx[8] = {-1,-1,-1, 0,0, 1,1,1};
int dy[8] = {-1, 0, 1,-1,1,-1,0,1};

int bfs(int si, int sj) {
    int qx[MAXN * MAXN], qy[MAXN * MAXN];
    int head = 0, tail = 0;
    vis[si][sj] = true;
    qx[tail] = si; qy[tail] = sj; tail++;
    int area = 1;

    while (head < tail) {
        int x = qx[head], y = qy[head];
        head++;
        for (int k = 0; k < 8; ++k) {
            int nx = x + dx[k], ny = y + dy[k];
            if (nx >= 0 && nx < n && ny >= 0 && ny < m &&
                !vis[nx][ny] && grid[nx][ny] == 'I') {
                vis[nx][ny] = true;
                qx[tail] = nx; qy[tail] = ny; tail++;
                area++;
            }
        }
    }
    return area;
}

int main(void) {
    if (scanf("%d %d", &n, &m) != 2) return 0;
    for (int i = 0; i < n; ++i) {
        if (scanf("%s", grid[i]) != 1) return 0;
    }

    int islands = 0, max_area = 0;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            if (!vis[i][j] && grid[i][j] == 'I') {
                islands++;
                int area = bfs(i, j);
                if (area > max_area) max_area = area;
            }
        }
    }
    printf("%d %d\n", islands, max_area);
    return 0;
}