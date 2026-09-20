#include<stdio.h>
#include<stdlib.h>
#include<string.h>
int main(void){
    int a;
    long long int max=0,maxi;
    scanf("%d",&a);
    char t[20],c[20];
    scanf("%s",t);
    int b=strlen(t)-a;
    for(int i=0;i<=strlen(t)-b;i++){
        memcpy(c,&t[i],b);
        maxi=atoll(c);
        for(int d=0;d<strlen(t);d++){
            char x[2];
            memcpy(x,&t[d],1);
            if(d<i||d>=i+b)maxi+=atoll(x);
        }
        if(maxi>max)max=maxi;
    }
    printf("%lld",max);
    return 0;
}