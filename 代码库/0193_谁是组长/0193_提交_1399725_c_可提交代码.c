#include <stdio.h>
#include <stdlib.h>

int main(void) {
    long long n;
    int m;
    if (scanf("%lld %d", &n, &m) != 2) return 0;

    if (m <= 0) {
        puts("-1");
        return 0;
    }

    long long *votes = (long long *)malloc(sizeof(long long) * m);
    if (!votes) return 0;

    for (int i = 0; i < m; ++i) {
        if (scanf("%lld", &votes[i]) != 1) {
            free(votes);
            return 0;
        }
    }

    // Boyer-Moore 投票找候选
    long long cand = 0;
    int cnt = 0;
    for (int i = 0; i < m; ++i) {
        if (cnt == 0) {
            cand = votes[i];
            cnt = 1;
        } else if (votes[i] == cand) {
            cnt++;
        } else {
            cnt--;
        }
    }

    // 验证是否过半
    int occ = 0;
    for (int i = 0; i < m; ++i) {
        if (votes[i] == cand) occ++;
    }

    if (occ > m / 2) {
        printf("%lld\n", cand);
    } else {
        puts("-1");
    }

    free(votes);
    return 0;
}