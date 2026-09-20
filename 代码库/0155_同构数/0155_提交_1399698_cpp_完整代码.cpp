#include<stdio.h>
#include<math.h>
int main(void){
    int a,b;
    scanf("%d%d",&a,&b);
    long long int sum=0;
    for(int i=a;i<b+1;i++){
        if(i*(i-1)%(int)pow(10,(int)log10(i)+1)==0)sum+=i;
    }
    printf("%lld\n",sum);
    return 0;
}