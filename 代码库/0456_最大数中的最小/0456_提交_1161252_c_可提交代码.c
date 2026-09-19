#include<stdio.h>
#include<limits.h>
int main(void){
    int n,m,max;
    scanf("%d%d",&n,&m);
    int t[n][m];
    int a=0,b=0;
    while(a<n){
        while(b<m){
            scanf("%d",&t[a][b]);
            b++;
        }
        a++;
        b=0;
    }
    int x,y,p=INT_MAX;
    a=0;b=0;
    while(a<n){
        x=t[a][0];
        while(b<m){
            
            y=t[a][b];
            if(y>x)x=y;
            b++;
        }
        b=0;
        a++;
        if(x<p)p=x;
    }
    printf("%d",p);
    return 0;
}