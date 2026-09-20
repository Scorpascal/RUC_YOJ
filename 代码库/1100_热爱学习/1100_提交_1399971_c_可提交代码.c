#include <stdio.h>
#include <stdlib.h>

int main() {
    int N;
    long long M;
    if (scanf("%d %lld", &N, &M) != 2) return 0;
    long long *A = (long long*)malloc(sizeof(long long)*N);
    long long *B = (long long*)malloc(sizeof(long long)*N);
    long long mx = 0;
    for (int i=0;i<N;i++){ scanf("%lld",&A[i]); if (A[i]>mx) mx=A[i]; }
    for (int i=0;i<N;i++){ scanf("%lld",&B[i]); if (B[i]>mx) mx=B[i]; }
    long long high = M * mx;
    long long low = 0;
    long long totalSlots = (long long)N * M;
    while (low < high) {
        long long mid = (low + high + 1) >> 1;
        long long need = 0;
        int ok = 1;
        for (int i=0;i<N;i++){
            long long k;
            if (B[i] >= A[i]) {
                // 自学收益更高，全部用 Bi
                k = (mid + B[i] - 1) / B[i];
            } else {
                // 先用最多 M 次上课 Ai
                long long cap = M * A[i];
                if (mid <= cap) {
                    k = (mid + A[i] - 1) / A[i];
                } else {
                    k = M + (mid - cap + B[i] - 1) / B[i];
                }
            }
            need += k;
            if (need > totalSlots){ ok = 0; break; }
        }
        if (ok) low = mid; else high = mid - 1;
    }
    printf("%lld\n", low);
    free(A); free(B);
    return 0;
}