#include<stdio.h>
#include<string.h>
int main(void){
    char t[105];
    fgets(t, 101, stdin);
    int len = strlen(t);
    if (t[len-1] == '\n') {
        t[len-1] = '\0';
        len--;
    }
    char x=t[strlen(t)-1];
    char m[strlen(t)+1];
    m[0]=x;
    for(int i=0;i<strlen(t)-1;i++){
        m[i+1]=t[i];
    }
    m[strlen(t)]='\0';
    puts(m);
    return 0;

    
}