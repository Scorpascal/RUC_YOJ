#include<stdio.h>
int main(void){
    int sum;
    scanf("%d",&sum);
    int a,b,c,d,e,flag=1;
    for(int a=1;a<6&&flag;a++){
        for(int b=1;b<6&&flag;b++){
            if(a!=b){
                for(int c=1;c<6&&flag;c++){
                    if(b!=c&&a!=c){
                        for(int d=1;d<6&&flag;d++){
                            if(c!=d&&b!=d&&a!=d){
                                for(int e=1;e<6&&flag;e++){
                                    if(sum==a+4*b+6*c+4*d+e&&a!=e&&b!=e&&c!=e&&d!=e){
                                        printf("%d %d %d %d %d\n",a,b,c,d,e);
                                        flag--;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}