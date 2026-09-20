#include<stdio.h>
int f(int a){
    if(a==1) return 1;
    else if(a==2) return 2;
    else return f(a-1)+f(a-2);
}
int main(void){
    int i;
    scanf("%d",&i);
    printf("%d",f(i));
    return 0;
}