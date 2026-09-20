#include<stdio.h>
void dijian(long long int*x,int y){
    long long int c;
    for(int i=0;i<y-1;){
        if(x[i]<x[i+1]){
            c=x[i];
            x[i]=x[i+1];
            x[i+1]=c;
            if(i>0)i--;
        }
        else i++;
    }
}
int main(void){
    int n,x,y,z;
    scanf("%d%d%d%d",&n,&x,&y,&z);
    long long int all[n];
    for(int i=0;i<n;i++){
        scanf("%lld", &all[i]);
    }
    dijian(all,n);
    // 一等奖
    for(int i=0;i<x;i++){
        if(i) printf(" ");
        printf("%lld",all[i]);
    }
    printf("\n");
    // 二等奖
    for(int i=x;i<y;i++){
        if(i>x) printf(" ");
        printf("%lld",all[i]);
    }
    printf("\n");
    // 三等奖（逆序输出）
    for(int i=z-1;i>=y;i--){
        if(i<z-1) printf(" ");
        printf("%lld",all[i]);
    }
    printf("\n");
    return 0;
}