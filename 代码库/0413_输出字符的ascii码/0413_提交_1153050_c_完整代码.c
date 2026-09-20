#include<stdio.h>
int main(void){
    char a,b;
    if (scanf(" %c %c", &a, &b) != 2) {  
        return 0;
    }
    printf("%c,%c\n%d,%d\n", a, b, (unsigned char)a, (unsigned char)b); 
    return 0;
}