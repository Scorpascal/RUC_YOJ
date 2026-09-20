#include<stdio.h>
#include<stdlib.h>
#include<string.h>
void f(int n,int now,int*a,int*b,int**t){
    if(now==n){
        int flag=2;
        for(int j=0;j<n;j++){//注意先按列枚举再按行枚举,防止头部的1在第一行被统计完使得flag直接小于0
            flag=2;
            if(t[0][j]==1){
                    flag--;
                }
            for(int i=1;i<n;i++){
                if(t[i][j]>0&&t[i-1][j]==0){
                    flag--;
                    if(flag==0){
                        return;
                    }
                }
            }
        }
        if(flag){
            for(int a1=0;a1<n;a1++){
                for(int b1=0;b1<n;b1++){
                    printf("%d ",t[a1][b1]);
                }
                printf("\n");
            }
            free(a);
            free(b);
            for(int i=0;i<n;i++){
                free(t[i]);
            }
            free(t);
            exit(0);//找到答案直接在释放内存后终止程序
        }
        return;
    }
    for(int i=0;i+a[now]<=n;i++){
        for(int j=i;j<i+a[now];j++){
            if(b[j]>0){
                b[j]--;
            }
            else{
                for(int k=i;k<j;k++){//注意这里是k<j而不是k<=j,因为b[j]并没有执行-1操作
                    b[k]++;
                }
                break;
            }
            if(j==i+a[now]-1){
                for(int k=i;k<=j;k++){//而这里是<=
                    t[now][k]++;
                }
                f(n,now+1,a,b,t);
                for(int k=i;k<=j;k++){
                    b[k]++;
                    t[now][k]--;
                }
            }
        }
    }
}
int main(void){
    int n;scanf("%d",&n);
    int*a=(int*)malloc(n*sizeof(int));
    int*b=(int*)malloc(n*sizeof(int));
    for(int i=0;i<n;i++){
        scanf("%d",a+i);
    }
    for(int i=0;i<n;i++){
        scanf("%d",b+i);
    }
    int**t=(int**)malloc(n*sizeof(int*));
    for(int i=0;i<n;i++){
        t[i]=(int*)malloc(n*sizeof(int));
        memset(t[i],0,n*sizeof(int));
    }
    f(n,0,a,b,t);
    return 0;
}