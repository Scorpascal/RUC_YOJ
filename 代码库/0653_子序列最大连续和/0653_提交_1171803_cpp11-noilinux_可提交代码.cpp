#include <stdio.h>
int main(void) {
    int n;
    if(scanf("%d",&n)!=1)perror("错误类型:");
    int a[n];
    for(int i = 0; i < n; i++) {
        if(scanf("%d", &a[i])!=1)perror("错误类型:");
    }
    int max_sum = a[0], curr_sum = a[0];
    for(int i = 1; i < n; i++) {
        if(curr_sum < 0) curr_sum = a[i];
        else curr_sum += a[i];
        if(curr_sum > max_sum) max_sum = curr_sum;
    }
    printf("%d\n", max_sum);
    return 0;
}