#include <stdio.h>
int main() {
    int T;
    if (scanf("%d", &T) != 1) return 0;
    while (T--) {
        long long N;
        scanf("%lld", &N);
        if ((N & 1LL) || (N % 4 == 0)) {
            printf("YES\n");
        } else {
            printf("NO\n");
        }
    }
    return 0;
}//1605AK