#include <stdio.h>

int rev_int(int x){
    int r = 0;
    while (x > 0){
        r = r * 10 + x % 10;
        x /= 10;
    }
    return r;
}

int main(void){
    int N, M;
    if (scanf("%d %d", &N, &M) != 2) return 0;
    int count = 0;
    for (int A = 1; A < N; ++A){
        for (int B = A; B < N; ++B){
            int C = A + B;
            if (C <= M || C >= N) continue;
            if (rev_int(A) + rev_int(B) == rev_int(C)) ++count;
        }
    }
    printf("%d\n", count);
    return 0;
}