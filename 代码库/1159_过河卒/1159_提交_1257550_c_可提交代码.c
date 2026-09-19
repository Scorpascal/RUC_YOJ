#include<stdio.h>
int main(void){
    int n,m,a,b,c=0;
    scanf("%d%d%d%d",&n,&m,&a,&b);
    long long int t[n+1][m+1];
    for(int i=0;i<n+1;i++){
        if(c==0){
            t[i][0]=1;
        }
        if((i-a)*(i-a)+b*b==5||i==a&&0==b)c=1;
        if(c==1){
            t[i][0]=0;
        }
    }
    c=0;
    for(int i=0;i<m+1;i++){
        if(c==0){
            t[0][i]=1;
        }
        if(a*a+(i-b)*(i-b)==5||0==a&&i==b)c=1;
        if(c==1){
            t[0][i]=0;
        }
    }
    for(int i=1;i<n+1;i++){
        for(int j=1;j<m+1;j++){
            if((i-a)*(i-a)+(j-b)*(j-b)==5||i==a&&j==b)continue;
            else t[i][j]=0;
            if((i-1-a)*(i-1-a)+(j-b)*(j-b)!=5&&!(i-1==a&&j==b))t[i][j]+=t[i-1][j];
            if((i-a)*(i-a)+(j-1-b)*(j-1-b)!=5&&!(i==a&&j-1==b))t[i][j]+=t[i][j-1];
        }
    }
    printf("%lld",t[n][m]);
    return 0;
}