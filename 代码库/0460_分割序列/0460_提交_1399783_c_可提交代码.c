#include <stdio.h>
#include <stdlib.h>

int main() {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    long long a;
    long long total = 0;
    long long arr[1005];
    for (int i = 0; i < n; ++i) {
        scanf("%lld", &arr[i]);
        total += arr[i];
    }

    long long pref = 0;
    long long bestP = 0;
    long long bestDiff = -1;
    long long bestSign = -1; // sign of (2*P - total): >=0 means P>=Q

    for (int i = 1; i <= n + 1; ++i) {
        // Pi = pref (sum of first i-1), Qi = total - pref
        long long twoP_minus_total = 2 * pref - total;
        long long diff = twoP_minus_total >= 0 ? twoP_minus_total : -twoP_minus_total;
        long long sign = twoP_minus_total >= 0 ? 1 : 0;
        if (bestDiff == -1 || diff < bestDiff) {
            bestDiff = diff;
            bestP = pref;
            bestSign = sign;
        } else if (diff == bestDiff) {
            // 平局时优先 Pi >= Qi 的解
            if (sign == 1 && bestSign == 0) {
                bestP = pref;
                bestSign = sign;
            }
        }
        if (i <= n) pref += arr[i-1];
    }

    long long bestQ = total - bestP;
    printf("%lld %lld\n", bestP, bestQ);
    return 0;
}