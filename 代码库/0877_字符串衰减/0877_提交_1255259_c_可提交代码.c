#include<stdio.h>
#include<string.h>
#include<stdlib.h>
int k=1;
void f(char*t,int len,int n){
    if(n==k-1){
        puts(t);
    }
    else{
        char t1[(len+1)/2+1];
        memset(t1,0,sizeof(t1));
        int temp=0;
        if(k%2==0){
            for(int i=0;i<len;i++){
                if(i%2==0){
                    t1[temp++]=t[i];
                }
            }
        }
        else{
            for(int i=0;i<len;i++){
                if(i%2==1){
                    t1[temp++]=t[i];
                }
            }
        }
        k++;
        f(t1,strlen(t1),n);
    }
}
int main(void){
    char t[3005];
    fgets(t,3001,stdin);
    int n;
    scanf("%d",&n);
    f(t,strlen(t),n);
    return 0;
}