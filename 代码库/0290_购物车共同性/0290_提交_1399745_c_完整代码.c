#include<stdio.h>
#include<stdlib.h>
int f(const void*a,const void*b){
    return *(int*)a-*(int*)b;
}
int main(void){
    int n;
    scanf("%d",&n);
    int a1;
    scanf("%d",&a1);
    int t[a1][2];
    for(int i=0;i<a1;i++){
        scanf("%d",*(t+i));
        t[i][1]=1;
    }
    for(int i=0;i<n-1;i++){
        int j;
        scanf("%d",&j);
        for(int k=0;k<j;k++){
            int temp;
            scanf("%d",&temp);
            for(int u=0;u<a1;u++){
                if(temp==t[u][0]){
                    t[u][1]++;
                }
            }
        }
    }
    int answer[a1];
    int sum=0;
    for(int i=0;i<a1;i++){
        if(t[i][1]==n)answer[sum++]=t[i][0];
    }
    if(sum==0)printf("NO\n");
    else{
        qsort(answer,sum,sizeof(int),f);
        for(int i=0;i<sum;i++){
            printf("%d ",answer[i]);
        }
        printf("\n");
    }
    return 0;
}