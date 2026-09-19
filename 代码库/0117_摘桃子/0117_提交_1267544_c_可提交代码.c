#include<stdio.h>
int main(void){
    int n;scanf("%d",&n);
    int t[n][n];
    for(int i=0;i<n;i++){
        for(int j=0;j<=i;j++){
            scanf("%d",&t[i][j]);
        }
    }
    for(int i=n-2;i>=0;i--){
        for(int j=0;j<=i;j++){
            t[i][j]+=t[i+1][j]>t[i+1][j+1]?t[i+1][j]:t[i+1][j+1];
        }
    }
    printf("%d",t[0][0]);
    return 0;
}