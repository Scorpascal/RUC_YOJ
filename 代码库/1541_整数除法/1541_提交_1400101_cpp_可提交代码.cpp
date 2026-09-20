#include<stdio.h>
int main(void){
    int a,b;
    scanf("%d%d",&a,&b);
    double c=(double)a/b;
    printf("%.2lf",c);
    return 0;
}