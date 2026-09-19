#include<stdio.h>
#include<limits.h>
#include<stdlib.h>
int min1=INT_MAX;
int min2=INT_MAX;
void f1(int**t,int n1,int n2,int now,int n){
    if(t[n1][n2]==-1)return;
    now++;
    if(now>min1)return;
    if(t[n1][n2]==1)return;
    else if(t[n1][n2]==3){
        if(now<min1)min1=now;
    }
    else if(t[n1][n2]==4||t[n1][n2]==0||t[n1][n2]==2){
        t[n1][n2]=-1;
        if(n1>0)f1(t,n1-1,n2,now,n);
        if(n2>0)f1(t,n1,n2-1,now,n);
        if(n1<n-1)f1(t,n1+1,n2,now,n);
        if(n2<n-1)f1(t,n1,n2+1,now,n);
        t[n1][n2]=0;
    }
    else return;
}
void f2(int**t,int n1,int n2,int now,int n){
    if(t[n1][n2]==-1)return;
    now++;
    if(now>min2)return;
    if(t[n1][n2]==1)return;
    else if(t[n1][n2]==4){
        if(now<min2)min2=now;
    }
    else if(t[n1][n2]==3||t[n1][n2]==0||t[n1][n2]==2){
        t[n1][n2]=-1;
        if(n1>0)f2(t,n1-1,n2,now,n);
        if(n2>0)f2(t,n1,n2-1,now,n);
        if(n1<n-1)f2(t,n1+1,n2,now,n);
        if(n2<n-1)f2(t,n1,n2+1,now,n);
        t[n1][n2]=0;
    }
    else return;
}
int main(void){
    int n,a1[2],a2[2],a3[2];
    scanf("%d",&n);
    int**t=(int**)malloc(n*sizeof(int*));
    int**t1=(int**)malloc(n*sizeof(int*));
    for(int i=0;i<n;i++){
        *(t+i)=(int*)malloc(n*sizeof(int));
        *(t1+i)=(int*)malloc(n*sizeof(int));
        for(int j=0;j<n;j++){
            scanf("%d",*(t+i)+j);
            t1[i][j]=t[i][j];
            if(t[i][j]==2){
                a1[0]=i;
                a1[1]=j;
            }
            if(t[i][j]==3){
                a2[0]=i;
                a2[1]=j;
            }
            if(t[i][j]==4){
                a3[0]=i;
                a3[1]=j;
            }
        }
    }
    f1(t,a1[0],a1[1],0,n);
    f2(t1,a2[0],a2[1],0,n);
    printf("%d",min1+min2-2);
    return 0;
}