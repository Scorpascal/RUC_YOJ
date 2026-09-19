#include<stdio.h>
#include<stdlib.h>
int f(int a,int b){
    return (a+b+abs(a-b))/2;
}
int g(int a,int b){
    return (a+b-abs(a-b))/2;
}
int main(void){
    int a,b,c;
    scanf("%d,%d,%d",&a,&b,&c);
    printf("%d,%d,%d",g(g(a,b),g(b,c)),a+b+c-f(f(a,b),f(b,c))-g(g(a,b),g(b,c)),f(f(a,b),f(b,c)));
    return 0;
}