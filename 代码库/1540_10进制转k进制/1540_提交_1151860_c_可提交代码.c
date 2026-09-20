#include<stdio.h>
int main(void){
    int x,k;
    scanf("%d%d",&x,&k);
    int a[4]={0,0,0,0};
    for(int i=1;i<5;i++){
        a[4-i]=x%k;
        x/=k;
        if(x==0)break;
    }
    printf("%d%d%d%d",a[0],a[1],a[2],a[3]);
    return 0;
}