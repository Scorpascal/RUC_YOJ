#include <stdio.h>
#include <string.h>
#include <math.h>

#define EPS 1e-6
#define MAXN 4

typedef struct {
    double v;
    char expr[64];
} Node;

int n;
Node a[MAXN];

int dfs(int cnt, Node arr[]) {
    if (cnt == 1) {
        if (fabs(arr[0].v - 24.0) < EPS) {
            printf("%s", arr[0].expr);
            return 1;
        }
        return 0;
    }
    for (int i = 0; i < cnt; ++i) {
        for (int j = 0; j < cnt; ++j) {
            if (i == j) continue;
            Node b[MAXN];
            int idx = 0;
            for (int k = 0; k < cnt; ++k) if (k != i && k != j) b[idx++] = arr[k];

            double x = arr[i].v, y = arr[j].v;
            char ex[64], ey[64];
            strcpy(ex, arr[i].expr);
            strcpy(ey, arr[j].expr);

            // +
            b[idx].v = x + y;
            snprintf(b[idx].expr, sizeof(b[idx].expr), "(%s+%s)", ex, ey);
            if (dfs(idx + 1, b)) return 1;

            // -
            b[idx].v = x - y;
            snprintf(b[idx].expr, sizeof(b[idx].expr), "(%s-%s)", ex, ey);
            if (dfs(idx + 1, b)) return 1;

            // *
            b[idx].v = x * y;
            snprintf(b[idx].expr, sizeof(b[idx].expr), "(%s*%s)", ex, ey);
            if (dfs(idx + 1, b)) return 1;

            // /
            if (fabs(y) > EPS) {
                b[idx].v = x / y;
                snprintf(b[idx].expr, sizeof(b[idx].expr), "(%s/%s)", ex, ey);
                if (dfs(idx + 1, b)) return 1;
            }
        }
    }
    return 0;
}

int main(void) {
    for (int i = 0; i < 4; ++i) {
        if (scanf("%lf", &a[i].v) != 1) return 0;
        snprintf(a[i].expr, sizeof(a[i].expr), "%.0f", a[i].v);
    }
    if (!dfs(4, a)) printf("No Answer");
    return 0;
}