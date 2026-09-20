#include<stdio.h>
#include<stdlib.h>
#include<limits.h>
#include<string.h>
int sum=0;
void f(int*t,int n,int a){
    if(a==n){
        sum++;
        return;
    }
    else if(a==0){
        for(int i=0;i<n;i++){
            t[a]=i;
            f(t,n,a+1);
        }
        return;
    }
    else{
        for(int i=0;i<n;i++){
            for(int j=0;j<a;j++){
                if(i==t[j])break;
                if(a-j==abs(i-t[j]))break;
                if(j==a-1){
                    t[a]=i;
                    f(t,n,a+1);
                }
            }
        }
    }
}
int main(void){
    int n;
    scanf("%d",&n);
    int*t=(int*)malloc(n*sizeof(int));
    f(t,n,0);
    printf("%d\n",sum);
    free(t);
    return 0;
}