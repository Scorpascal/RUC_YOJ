#include<stdio.h>
int comp(int a,int b){
    if(a==b)return 100;//区分不变与自变
    else if(a==0){
        if(b==0||b==6||b==9)return 0;
        if(b==8)return 1;//添加用1,减少用-10
    }
    else if(a==1){
        if(b==1)return 0;
        if(b==7)return 1;
    }
    else if(a==2){
        if(b==2||b==3)return 0;
    }
    else if(a==3){
        if(b==3||b==2||b==5)return 0;
        if(b==9)return 1;
    }
    else if(a==4){
        if(b==4)return 0;
    }
    else if(a==5){
        if(b==5||b==3)return 0;
        if(b==6||b==9)return 1;
    }
    else if(a==6){
        if(b==6||b==0||b==9)return 0;
        if(b==8)return 1;
        if(b==5)return -10;//区分0+0与1-10
    }
    else if(a==7){
        if(b==7)return 0;
        if(b==1)return -10;
    }
    else if(a==8){
        if(b==8)return 0;
        if(b==0||b==6||b==9)return -10;
    }
    else if(a==9){
        if(b==9||b==0||b==6)return 0;
        if(b==8)return 1;
        if(b==5||b==3)return -10;
    }
    return 100000000;
}
int main(void){
    int a,b,c;
    char x;
    scanf("%d%c%d=%d",&a,&x,&b,&c);
    for(int i=0;i<10;i++){
        for(int j=0;j<10;j++){
            for(int k=0;k<10;k++){
                if(x=='+'){
                    if((i+j)==k&&((comp(a,i)+comp(b,j)+comp(c,k))==91)||(i+j)==k&&((comp(a,i)+comp(b,j)+comp(c,k))==200)||(i-j==k)&&(comp(a,i)+comp(b,j)+comp(c,k))==201){
                        if((i+j)==k&&((comp(a,i)+comp(b,j)+comp(c,k))==91)||(i+j)==k&&((comp(a,i)+comp(b,j)+comp(c,k))==200)){
                            printf("%d+%d=%d\n",i,j,k);
                        }
                        else printf("%d-%d=%d\n",i,j,k);
                    }
                }
                if(x=='-'){
                    if((i-j)==k&&((comp(a,i)+comp(b,j)+comp(c,k))==91)||(i-j)==k&&((comp(a,i)+comp(b,j)+comp(c,k))==200)||(i+j==k)&&(comp(a,i)+comp(b,j)+comp(c,k))==190){
                        if((i-j)==k&&((comp(a,i)+comp(b,j)+comp(c,k))==91)||(i-j)==k&&((comp(a,i)+comp(b,j)+comp(c,k))==200)){
                            printf("%d-%d=%d\n",i,j,k);
                        }
                        else printf("%d+%d=%d\n",i,j,k);
                    }
                }
            }
        }
    }
    return 0;
}