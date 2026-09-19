#include<stdio.h>
int main(void){
    int m,n,i,x,y;
    scanf("%d%d",&n,&m);
    for(x=1;x<=n;x++){
        y=x;
        for(i=0;i<m;i++){
            printf("%d ",y);
            y+=x;
        }
        printf("\n");
    }
    return 0;
}