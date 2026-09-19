#include<stdio.h>
#include<string.h>
void accu(int temp,int a1,int b1,char a[],char b[],int c1){
    char d1[2005];
    memset(d1,'0',2005);//不能写成char d1[2005]={'0'};这样只会默认分配第一个元素的值
    int lena=strlen(a);
    int lenb=strlen(b);
    int maxlen=lena>lenb?lena:lenb;
    int minlen=lena<lenb?lena:lenb;
    if(a1+b1+c1==-3){
        accu(temp,1,1,b,a,-1);
        return;//必须return防止多次打印,造成全为0的数组越界
    }
    else if(a1+b1+c1==-1){
        if(a1==1){
            accu(temp,1,1,a,b,1);
            return;
        }
        else{
            accu(-temp,1,1,a,b,1);
            return;
        }
    }
    else if(a1+b1+c1==1){
        if(a1==-1){
            accu(temp,1,1,b,a,-1);
            return;
        }
        else{
            if(lena<lenb)accu(-temp,1,1,b,a,-1);
            else if(lena==lenb&&strcmp(a,b)<0){
                accu(-temp,1,1,b,a,-1);
                return;
            }
            else if(lena==lenb&&strcmp(a,b)==0){
                printf("0\n");
                return;
            }
            else{
                for(int i=0;i<lena;i++){
                    if(i<lenb){
                        d1[2004-i]+=a[lena-1-i]-b[lenb-1-i];//必须用+=,不然重新赋值进位失效
                        if(d1[2004-i]<'0'){
                            d1[2004-i]+=10;
                            d1[2003-i]--;
                        }
                    }
                    else{
                        d1[2004-i]+=a[lena-1-i]-'0';
                    }
                }
            }
        }
    }
    else{
        if(lena<lenb){
            accu(temp,1,1,b,a,1);
            return;
        }
        for(int i=0;i<lena;i++){
            if(i<lenb){
                d1[2004-i]+=a[lena-1-i]-'0'+b[lenb-1-i]-'0';
                if(d1[2004-i]>'9'){
                    d1[2004-i]-=10;
                    d1[2003-i]++;
                }
            }
            else{
                d1[2004-i]+=a[lena-1-i]-'0';
            }
        }   
    }
    if(temp==-1)printf("-");
    int i=0;
    while(d1[i]=='0')i++;
    for(;i<2005;i++)printf("%c",d1[i]);
    printf("\n");
}
int main(void){
    char c,a[2005]={'\0'},b[2005]={'\0'};
    int c1=1,a1=1,b1=1,temp=1;
    scanf(" %c",&c);if(c=='-')c1=-1;
    scanf("%s\n%s",a,b);
    int lena=strlen(a);
    int lenb=strlen(b);
    if(a[0]=='-'){
        a1=-1;
        for(int i=0;i<lena;i++){
            a[i]=a[i+1];
        }
        a[lena-1]='\0';
        lena--;
    }
    if(b[0]=='-'){
        b1=-1;
        for(int i=0;i<lenb;i++){
            b[i]=b[i+1];
        }
        b[lenb-1]='\0';
        lenb--;
    }
    accu(temp,a1,b1,a,b,c1);    
    return 0;
}
