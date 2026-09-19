#include<stdio.h>
int main(void){
    int n;
    scanf("%d",&n);
    int t[n];
    int i=0;
    while(i<n){
        scanf("%d",&t[i++]);
    }
    int ver=0;
    for(i=0;i<n-1;){
        if(t[i]>t[i+1]){
            ver=t[i];
            t[i]=t[i+1];
            t[i+1]=ver;
            i=0;
        }
        else i++;
    }
    i=0;
    while(i<n){
        printf("%d ",t[i++]);
    }
    return 0;
}