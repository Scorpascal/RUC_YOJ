#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>
int f(const void*a,const void*b){
    if(*(long long int*)a>*(long long int*)b)return 1;
    else if(*(long long int*)a<*(long long int*)b)return -1;
    else return 0;
}//注意不能直接返回两数相减,防止long long int转变为int时溢出
int main(void){
    int n;
    scanf("%d",&n);
    char b[5];
    scanf("%s",b);
    long long int t[n];
    long long int t1[n];
    memset(t,0,sizeof(t));
    memset(t1,0,sizeof(t1));
    char a[n][25];
    for(int i=0;i<n;i++){
        scanf("%s",*(a+i));
        for(int j=0;j<strlen(a[i]);j++){
            for(int k=1;k<5;k++){
                if(a[i][j]==b[k-1]){
                    t[i]+=k*(long long int)pow(10,strlen(a[i])-j);
                    t1[i]+=k*(long long int)pow(10,strlen(a[i])-j);
                }
            }
        }
    }
    qsort(t1,n,sizeof(long long int),f);
    for(int i=0;i<n;i++){
        for(int j=0;j<n;j++){
            if(t1[i]==t[j]){
                puts(a[j]);
                break;
            }
        }
    }
    return 0;
}