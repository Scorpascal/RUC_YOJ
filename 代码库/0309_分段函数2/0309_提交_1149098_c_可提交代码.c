#include<stdio.h>
#include<math.h>
int main(void){
    float a;
    scanf("%f",&a);
    if(a<0)printf("%.2f",-a);
    else if(a>1)printf("%.2f",a*a);
    else printf("%.2f",sqrt(a));
    return 0;
}