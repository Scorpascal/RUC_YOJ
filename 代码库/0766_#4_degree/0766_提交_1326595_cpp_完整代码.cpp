#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>
#include<time.h>
int max=INT_MIN;
void f(int**t,int n,int m,int nowstep,int nowvalue){
    if(nowstep==m){
        if(nowvalue>max)max=nowvalue;
    }
    else{
        for(int i=0;i<n;i++){
            for(int j=0;j<n;j++){
                int a=i;int b=j;
                int temp=0;
                int u[n][n];
                memset(u,0,sizeof(u));
                while(a>=0&&a<n&&b>=0&&b<n){
                    temp++;
                    u[a][b]++;
                    t[a][b]++;
                    t[a][b]%=4;
                    if(t[a][b]==0)a--;
                    else if(t[a][b]==1)b++;
                    else if(t[a][b]==2)a++;
                    else b--;
                }
                f(t,n,m,nowstep+1,nowvalue+temp);
                for(int c=0;c<n;c++){
                    for(int d=0;d<n;d++){
                        while(u[c][d]>0){
                            t[c][d]+=3;
                            t[c][d]%=4;
                            u[c][d]--;
                        }
                    }
                }
            }
        }
    }
}
int main(void){
    int n,m;
    scanf("%d%d",&n,&m);
    int**t=(int**)malloc(n*sizeof(int*));
    for(int i=0;i<n;i++){
        *(t+i)=(int*)calloc(n,sizeof(int));
        for(int j=0;j<n;j++){
            scanf("%d",*(t+i)+j);
        }
    }
    f(t,n,m,0,0);
    printf("%d",90*max);
}