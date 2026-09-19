#include <stdio.h>
#include <string.h>

int main() {
    int L, M;
    if (scanf("%d %d", &L, &M) != 2) return 0;
    int removed[10005] = {0}; /* L <= 10000 */
    for (int i = 0; i < M; ++i) {
        int a, b;
        if (scanf("%d %d", &a, &b) != 2) return 0;
        if (a > b) { int t = a; a = b; b = t; }
        if (a < 0) a = 0;
        if (b > L) b = L;
        for (int x = a; x <= b; ++x) removed[x] = 1;
    }
    int cnt = 0;
    for (int i = 0; i <= L; ++i) if (!removed[i]) ++cnt;
    printf("%d\n", cnt);
    return 0;
}