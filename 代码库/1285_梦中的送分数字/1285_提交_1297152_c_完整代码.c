#include<stdio.h>
#include<string.h>
int main(void){
    long long int m,n;
    scanf("%lld%lld",&m,&n);
    long long int t[10];
    memset(t,0,sizeof(t));
    for(long long int i=m;i<=n;i++){
        long long int j=i;
        while(j>0){
            t[j%10]++;
            j/=10;
        }
    }
    for(int i=0;i<10;i++){
        printf("%lld ",t[i]);
    }
    return 0;
}