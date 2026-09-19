#include <stdio.h>
#include <math.h>

struct Point {
    double x;
    double y;
};

int main() {
    struct Point p1, p2;
    
    // 输入两个点的坐标
    scanf("%lf %lf", &p1.x, &p1.y);
    scanf("%lf %lf", &p2.x, &p2.y);

    double dx = p1.x - p2.x;
    double dy = p1.y - p2.y;
    double dist = sqrt(dx*dx + dy*dy);
    printf("%.3f\n", dist);

    return 0;
}
