#include <stdio.h>
#include <stdlib.h>

int main(void) {
    char s[16];
    if (scanf("%15s", s) != 1) return 0;
    int n = atoi(s);
    int unit = (n - 1) / 12 + 1;
    int pos = n - (unit - 1) * 12;
    int floor = (pos + 1) / 2;
    int room = (pos % 2 == 1) ? 1 : 2;
    printf("%d-%d0%d\n", unit, floor, room);
    return 0;
}