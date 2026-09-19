#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>
int min=INT_MAX;
void f(int**t,int*ans,int n,int*temp,int size,int s,int d,int val){
    if(s==d){
        if(val<min){
            min=val;
            for(int i=0;i<size;i++){
                ans[i]=temp[i];
            }
            ans[size]=-1;
        }
    }
    else{
        for(int i=0;i<n;i++){
            int flag=0;
            for(int j=0;j<size;j++){
                if(i==temp[j]){
                    flag=1;break;
                }
            }//防止出现环
            if(t[s][i]!=-1&&val<min&&flag==0){
                temp[size]=i;
                f(t,ans,n,temp,size+1,i,d,val+t[s][i]);
            }
        }
    }
}
int main(void){
    int n,s,d;scanf("%d%d%d",&n,&s,&d);
    int**t=(int**)malloc(n*sizeof(int*));
    int*ans=(int*)malloc(n*sizeof(int));
    int*temp=(int*)malloc(n*sizeof(int));
    memset(ans,-1,sizeof(int)*n);
    memset(temp,-1,sizeof(int)*n);
    for(int i=0;i<n;i++){
        t[i]=(int*)malloc(n*sizeof(int));
        for(int j=0;j<n;j++){
            scanf("%d",*(t+i)+j);
        }
    }
    f(t,ans,n,temp,0,s,d,0);
    if(ans[0]==-1) printf("-1\n");
    else{
        printf("%d->",s);
        int i=0;
        while(ans[i+1]!=-1)printf("%d->",ans[i++]);
        printf("%d\n",d);
    }
    return 0;
}