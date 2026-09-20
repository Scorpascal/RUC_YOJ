#include <stdio.h>

int main() {
    int d1, d2;
    const char* week[7] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
    scanf("%d %d", &d1, &d2);
    printf("%s\n", week[d1 % 7]);              // d1的后一天
    printf("%s\n", week[(d2 + 5) % 7]);        // d2的前一天
    return 0;
}