#include<stdio.h>
#include<math.h>
int main(void){
    float a,b;
    scanf("%f",&a);
    if(a<0)b=-a;
    else if(a>=0&&a<=1)b=sqrt(a);
    else b=a*a;
    printf("%.2f",b);
    return 0;
}