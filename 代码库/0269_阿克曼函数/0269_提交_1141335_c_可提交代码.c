#include<stdio.h>
int acm(int a,int b){
    if(a==0)return b+1;
    else if(b==0)return acm(a-1,1);
    else return acm(a-1,acm(a,b-1));
}
int main(void){
    int m,n;
    scanf("%d%d",&m,&n);
    printf("%d",acm(m,n));
    return 0;
}