#include<stdio.h>
#include<stdlib.h>
#include<string.h>
int*turn(char*x,int y){
    int*a=(int*)malloc(y*sizeof(int));
    for(int i=0;i<y;i++)a[i]=x[i]-'0';
    return a;
}
int main(void){
    char a[100000],b[100000],c[100000]={'0'};
    gets(a);
    gets(b);
    int lena,lenb,lenc;
    lena=strlen(a);
    lenb=strlen(b);
    lenc=lena+lenb;
    int c1[100000]={0};
    int* a1;
    int* b1;
    a1=turn(a,lena);
    b1=turn(b,lenb);
    for(int i=0;i<lenb;i++){
        for(int j=0;j<lena;j++){
            c1[lenc-1-i-j]+=a1[lena-1-j]*b1[lenb-1-i];
        }
    }
    for(int i=lenc-1;i>0;i--){
        c1[i-1]+=c1[i]/10;
        c1[i]%=10;
    }
    for(int i=0;i<lenc;i++)c[i]=c1[i]+'0';
    puts(c+(c[0]=='0'));
    free(a1);
    free(b1);
    return 0;
}