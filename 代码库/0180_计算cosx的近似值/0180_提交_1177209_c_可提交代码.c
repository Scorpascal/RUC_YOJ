#include <stdio.h>  
#include <math.h>//输出正弦余弦值
int main()
{
    double a;
    scanf("%lf",&a);
    getchar();
    printf("%lf\n",sin(a));
    printf("%lf\n",cos(a));
    return 0;
}