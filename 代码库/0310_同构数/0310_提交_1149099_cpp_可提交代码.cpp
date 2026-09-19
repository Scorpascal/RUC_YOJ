#include<stdio.h>
int main(void){
    int a,b,s=0;
    scanf("%d%d",&a,&b);
    for(int i=a;i<b+1;i++){
        if(i<10){if(i*(i-1)%10==0)s+=i;}
        else if(i<100){if(i*(i-1)%100==0)s+=i;}
        else if(i<1000){if(i*(i-1)%1000==0)s+=i;}
        else if(i<10000){if(i*(i-1)%10000==0)s+=i;}
        else continue;
    }
    printf("%d",s);
    return 0;
}