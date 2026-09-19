#include <stdio.h>
#include <limits.h>

int main() {
    int n, m, q;
    if (scanf("%d %d %d", &n, &m, &q) != 3) return 0;

    const long long INF = (long long)1e15;
    static long long dist[105][105];
    for (int i = 1; i <= n; ++i)
        for (int j = 1; j <= n; ++j)
            dist[i][j] = (i == j) ? 0 : INF;

    for (int i = 0; i < m; ++i) {
        int u, v, w;
        scanf("%d %d %d", &u, &v, &w);
        if (w < dist[u][v]) dist[u][v] = w;
    }

    for (int k = 1; k <= n; ++k)
        for (int i = 1; i <= n; ++i)
            if (dist[i][k] < INF)
                for (int j = 1; j <= n; ++j)
                    if (dist[k][j] < INF && dist[i][k] + dist[k][j] < dist[i][j])
                        dist[i][j] = dist[i][k] + dist[k][j];

    for (int i = 0; i < q; ++i) {
        int u, v;
        scanf("%d %d", &u, &v);
        if (dist[u][v] >= INF/2) printf("inf\n");
        else printf("%lld\n", dist[u][v]);
    }
    return 0;
}