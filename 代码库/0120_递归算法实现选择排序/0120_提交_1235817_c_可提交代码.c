#include<stdio.h>
#include<limits.h>
#include<stdlib.h>
int sum=0;
void f(int*t,int n,int p,int*ans){
    for(int i=0;i<n-1;i++){
        if(t[i+1]<t[i])break;
        if(i==n-2){
            for(int j=0;j<n;j++){
                ans[j]=t[j];
            }
            return;
        }  
    }
    for(int i=p+1;i<n;i++){
        if(t[i]<t[p]){
            break;
        }
        if(i==n-1){
            f(t,n,p+1,ans);
            return;
        }
    }
    sum++;
    int*temp=(int*)malloc(n*sizeof(int));
    int temp1=t[p],temp2=0;
    for(int i=0;i<n;i++){
        temp[i]=t[i];
    }
    //free(t);
    for(int i=p;i<n;i++){
        if(temp[i]<temp1){
            temp1=temp[i];
            temp2=i;
        }
    }
    int a=temp[p];
    temp[p++]=temp1;
    temp[temp2]=a;
    f(temp,n,p,ans);
    printf("%d<->%d:",p,temp2+1);
    for(int i=0;i<n;i++){
        printf("%d ",temp[i]);
    }
    free(temp);
    printf("\n");
}
int main(void){
    int n;
    scanf("%d",&n);
    int*t=(int*)malloc(n*sizeof(int));
    int*ans=(int*)malloc(n*sizeof(int));
    for(int i=0;i<n;i++){
        scanf("%d",t+i);
    }
    f(t,n,0,ans);
    printf("Total steps:%d\n",sum);
    printf("Right order:");
    for(int i=0;i<n;i++){
        printf("%d ",ans[i]);
    }
    printf("\n");
    free(t);
    free(ans);
    return 0;
}