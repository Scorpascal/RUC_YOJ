#include<stdio.h>
#include<string.h>
#include<stdlib.h>
void dizeng(int*x,int y){
    int c;
    for(int i=0;i<y-1;){
        if(x[i]>x[i+1]){
            c=x[i];
            x[i]=x[i+1];
            x[i+1]=c;
            if(i>0)i--;
        }
        else i++;
    }
}
void dijian(int*x,int y){
    int c;
    for(int i=0;i<y-1;){
        if(x[i]<x[i+1]){
            c=x[i];
            x[i]=x[i+1];
            x[i+1]=c;
            if(i>0)i--;
        }
        else i++;
    }
}
int main(void){
    int a,b=0,j=0;
    scanf("%d",&a);
    int*l=(int*)malloc(a*sizeof(int));
    int*y=(int*)malloc(a*sizeof(int));
    for (int i=0;i<a;i++)scanf("%d",y+i);
    for (int i=0;i<a;i++)scanf("%d",l+i);
    dizeng(y,a);
    dizeng(l,a);
    for(int i=0;i<a;i++){
        for(;j<a;){
            if(l[i]>y[j]){
                b++;j++;
            }
            break;
        }
    }
    printf("%d",b);
    free(y);
    free(l);
    return 0;
}