#include <stdio.h>
int main(void){
    int n;
    if (scanf("%d", &n) != 1) return 0;
    unsigned int u = (unsigned int)n;    
    printf("%15d\n", n);                  
    printf("%15u\n", u);                  
    printf("%15o\n", u);                  
    printf("%15x\n", u);                 
    return 0;
}