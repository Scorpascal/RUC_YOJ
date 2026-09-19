#include <stdio.h>

int main(void) {
    int x1,y1,x2,y2,x3,y3;
    if (scanf("%d %d %d %d %d %d", &x1,&y1,&x2,&y2,&x3,&y3) != 6) return 0;

    int ax = x1 + x2 - x3;
    int ay = y1 + y2 - y3;
    int bx = x1 + x3 - x2;
    int by = y1 + y3 - y2;
    int cx = x2 + x3 - x1;
    int cy = y2 + y3 - y1;

    printf("3\n");
    printf("%d %d\n", ax, ay);
    printf("%d %d\n", bx, by);
    printf("%d %d\n", cx, cy);

    return 0;
}