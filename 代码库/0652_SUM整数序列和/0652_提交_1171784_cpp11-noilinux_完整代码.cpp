#include<stdio.h>
int main(void){
    long long int n,sum=0;
    scanf("%lld",&n);
    if(n>=1)printf("%lld",(1+n)*n/2);
    else printf("%lld",(1+n)*(2-n)/2);
    return 0;
}