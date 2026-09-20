#include<stdio.h>
int main(void){
    int a,b,c=0;
    if(scanf("%d,%d",&a,&b)!=2){
        fprintf(stderr,"WRONG!!\n");
        return 1;
    }
    for(int i=a+1;i<b;i++){
        if(i%2!=0)c+=i;
    }
    printf("%d",c);
    return 0;
}