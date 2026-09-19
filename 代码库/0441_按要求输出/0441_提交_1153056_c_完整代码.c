#include<stdio.h>
int main(void){
    int a,b;
    if(scanf("a=%d,b=%d",&a,&b)!=2){
        return 0;
    }
    printf("a+b=%d",a+b);
    return 0;
}