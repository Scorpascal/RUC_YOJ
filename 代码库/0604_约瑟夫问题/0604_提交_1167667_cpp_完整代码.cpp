#include<stdio.h>
int main(void){
    int n,m,a,shu=1,s=0;
    scanf("%d%d",&n,&m);
    int t[10000]={0};
    for(a=1;;){
        if(!t[a]){
            if(shu%m==0){
                t[a]=a;
                s++;
                if(s==n)break;
            }
        shu++;
        }
        a++;
        if(a==n+1)a=1;
    }
    printf("%d",a);
    return 0;
}