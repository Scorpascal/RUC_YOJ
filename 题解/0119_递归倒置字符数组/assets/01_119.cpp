#include <stdio.h>
#include <string.h>

void f(char s[], int l, int r)
{
    ____qcodep____
    f(s,l+1,r-1);
}

int main(){
    char s[10001];
    scanf("%s", s);
    f(s,0,strlen(s)-1);
    printf("%s",s);
}

