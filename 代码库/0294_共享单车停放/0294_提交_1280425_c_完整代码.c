#include <stdio.h>
#include <math.h>

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    if (n <= 0 || n > 50) {
        printf("0\n");
        return 0;
    }

    int dist[55], remain[55], speed[55], timeMin[55];

    for (int i = 0; i < n; ++i) scanf("%d", &dist[i]);
    for (int i = 0; i < n; ++i) scanf("%d", &remain[i]);
    for (int i = 0; i < n; ++i) scanf("%d", &speed[i]);
    for (int i = 0; i < n; ++i) scanf("%d", &timeMin[i]);

    int bestIdx = -1;
    int bestDist = 0;

    for (int i = 0; i < n; ++i) {
        double periods = timeMin[i] / 10.0;
        long change = lround(speed[i] * periods); // 使用round规则
        long avail = (long)remain[i] + change;

        if (avail > 0) {
            if (bestIdx == -1 || dist[i] < bestDist) {
                bestIdx = i;
                bestDist = dist[i];
            }
        }
    }

    if (bestIdx == -1) {
        printf("0\n");
    } else {
        printf("%d\n", bestIdx + 1);
    }

    return 0;
}