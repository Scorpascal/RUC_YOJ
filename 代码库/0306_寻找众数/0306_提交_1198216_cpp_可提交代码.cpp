#include<stdio.h>
#include<limits.h>
int main(void){
    int a,b=-INT_MAX-1;
    scanf("%d",&a);
    int t[a][2];
    int m[a+1];
    for(int i=0;i<a;i++){
        scanf("%d",t+i);t[i][1]=1;
    }
    for(int i=0;i<=a;i++){
        m[i]=-INT_MAX-1;
    }
    for(int i=0;i<a-1;i++){
        for(int j=i+1;j<a;j++){
            if(t[i][0]==t[j][0]){
                t[i][1]+=t[j][1];
                t[j][1]=0;
            }
        }
    }
    for(int i=a;i>0;i--){
        if(i==1){
            printf("NO\n"); 
            return 0;
        }
        for(int j=0,x=0;j<a;j++){
            if(t[j][1]==i){
                m[a]=i;
                m[x++]=t[j][0];
            }
            if(m[a]!=0&&j==a-1)break;
        }
        if(m[a]!=-INT_MAX-1)break;
    }
    for(int i=0;i<a;i++){
        if(m[i]>b)b=m[i];
    }
    printf("%d %d\n",b,m[a]);
    return 0;
}