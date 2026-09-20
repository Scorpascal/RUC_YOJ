#include<stdio.h>
int main(void){
    int mtr,n;
    scanf("%d%d",&mtr,&n);
    int t[n];
    for(int i=0;i<n;i++){
        scanf("%d",t+i);
    }
    int sum=0;
    for(int i=0;i<n-1;i++){
        sum+=t[i+1]-t[i]<mtr?t[i+1]-t[i]:mtr;
    }
    printf("%d",sum+mtr);
    return 0;
}