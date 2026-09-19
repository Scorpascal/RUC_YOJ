#include<stdio.h>
#include<stdlib.h>
#include<string.h>
int f(const void*a,const void*b){
    return strcmp((char*)b,(char*)a);
}
int main(void){
    int n;
    scanf("%d",&n);
    char t[n][30];
    for(int i=0;i<n;i++){
        char temp[100];
        scanf("%99s",temp);
        sprintf(t[i],"%.8s%s",temp+6,temp);
    }
    qsort(t,n,sizeof(*t),f);
    for(int i=0;i<n;i++){
        puts(t[i]+8);
    }
    return 0;
}