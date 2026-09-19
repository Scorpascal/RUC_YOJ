#include<stdio.h>
#include<string.h>
int main(void){
    int k;scanf("%d",&k);char a[100],b[100];
    for(int i=1;i<k+1;i++){
        sprintf(a,"%d",i*i);
        for(int j=0;j<strlen(a);j++){
            b[j]=a[strlen(a)-1-j];
        }
        b[strlen(a)]='\0';
        if(!strcmp(a,b)){
            printf("%d*%d=%s\n",i,i,a);
        }
    }
    return 0;
}