#include<stdio.h>
/*#include<stdlib.h>
#include<limits.h>
int f(const char*a,const char*b){
    return(*(int*)a-*(int*)b);
}*/
int main(void){
    int n,k,t[100]={0},a,c,s=0;
    float b;
    scanf("%d%d",&n,&k);
    for(int i=0;i<n;i++){
        scanf("%d",&c);scanf("%d",&a);
        for(int j=0;j<a;j++){
            scanf("%f",&b);
            t[(int)(10*b)]++;
        }
    }
    for(int j=0;;j++){
        for(int i=11;i<=77;i++){
            if(t[i]==j){
                printf("%.1f %d\n",(float)i/10,j);
                if(++s==k)return 0;
            }
            if(i%10==7)i+=3;
        }
    }
    return 0;
}