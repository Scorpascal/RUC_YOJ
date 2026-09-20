#include<stdio.h>
#include<stdlib.h>
#include<string.h>
int main(void){
    int m,n;scanf("%d%d",&m,&n);
    int**t=(int**)malloc(m*sizeof(int*));
    for(int i=0;i<m;i++){
        *(t+i)=(int*)malloc(n*sizeof(int));
        for(int j=0;j<n;j++){
            scanf("%d",*(t+i)+j);
        }
    }
    t[0][0]=2;
    for(int k=0;k<m*n;k++){
        if(t[m-1][n-1]==2){
            printf("YES");
            return 0;
        }
        for(int i=0;i<m;i++){
            for(int j=0;j<n;j++){
                if(t[i][j]==2){
                    if(i>0&&t[i-1][j]==1)t[i-1][j]=2;
                    if(j>0&&t[i][j-1]==1)t[i][j-1]=2;
                    if(i<m-1&&t[i+1][j]==1)t[i+1][j]=2;
                    if(j<n-1&&t[i][j+1]==1)t[i][j+1]=2;
                }
            }
        }
    }
    printf("NO");
    return 0;
}