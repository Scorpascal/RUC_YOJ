#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<math.h>
int main(void){
    char t[10005];int a,c,e=0;
    long long int d;
    scanf("%10000s %d",t,&a);
    int b=strlen(t);
    for(int i=0;i<b;i++){
        if(t[i]=='X'||t[i]=='x'){
            t[i]='0';
            c=b-(i+1);break;
        }
    }
    d=atoll(t);
    for(int i=0;i<10;i++){
        if(i==0&&c==b-1)continue;
        if((d+i*(int)pow(10,c))%a==0)e++;
    }
    printf("%d",e);
    return 0;
}