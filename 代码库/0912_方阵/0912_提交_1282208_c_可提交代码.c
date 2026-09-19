#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define MAXN 205

static int n;
static int adj[MAXN][MAXN];
static int deg[MAXN];
static int g[MAXN][MAXN]; // adjacency list (row -> list of columns)
static int matchR[MAXN];  // matched row for column j (column->row), -1 if free
static int visC[MAXN];    // visited columns in DFS

bool dfs_augment(int r) {
    for (int k = 0; k < deg[r]; ++k) {
        int c = g[r][k];
        if (visC[c]) continue;
        visC[c] = 1;
        if (matchR[c] == -1 || dfs_augment(matchR[c])) {
            matchR[c] = r;
            return true;
        }
    }
    return false;
}

int max_matching() {
    // greedy + DFS augment
    for (int j = 0; j < n; ++j) matchR[j] = -1;
    int match = 0;

    // optional greedy to speed up
    for (int r = 0; r < n; ++r) {
        for (int k = 0; k < deg[r]; ++k) {
            int c = g[r][k];
            if (matchR[c] == -1) {
                matchR[c] = r;
                match++;
                break;
            }
        }
    }

    for (int r = 0; r < n; ++r) {
        // if row r is not matched in greedy, try DFS augment
        bool isMatched = false;
        for (int j = 0; j < n; ++j) {
            if (matchR[j] == r) { isMatched = true; break; }
        }
        if (isMatched) continue;

        memset(visC, 0, sizeof(visC));
        if (dfs_augment(r)) match++;
    }
    return match;
}

int main() {
    int T;
    if (scanf("%d", &T) != 1) return 0;
    while (T--) {
        if (scanf("%d", &n) != 1) return 0;
        // read matrix and build adjacency
        for (int i = 0; i < n; ++i) {
            deg[i] = 0;
            for (int j = 0; j < n; ++j) {
                int x;
                scanf("%d", &x);
                adj[i][j] = x;
                if (x == 1) g[i][deg[i]++] = j;
            }
        }

        int m = max_matching();
        if (m == n) {
            puts("Yes");
        } else {
            puts("No");
        }
    }
    return 0;
}