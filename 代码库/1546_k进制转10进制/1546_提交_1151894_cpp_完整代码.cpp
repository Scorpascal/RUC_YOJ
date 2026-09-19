#include <stdio.h>
int main(void) {
    long long int k,a,b,c,d;
    scanf("%d%d%d%d%d",&k,&a,&b,&c,&d);
    printf("%lld",d+c*k+b*k*k+a*k*k*k);
    return 0;
}