#include <stdio.h>
#include <math.h>

int main() {
    int n;
    scanf("%d", &n);
    int a[100000], cnt = 0;
    int i;
    for(i = 1; i * i <= n; ++i) {
        if(n % i == 0) {
            printf("%d ", i);
            if(i != n / i) a[cnt++] = n / i;
        }
    }
    for(int j = cnt - 1; j >= 0; --j) {
        printf("%d", a[j]);
        if(j > 0) printf(" ");
    }
    printf("\n");
    return 0;
}