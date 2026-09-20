#include<stdio.h>
#include<stdlib.h>
int f(const void*a,const void*b){
    if(*(double*)a>*(double*)b)return 1;
    else if(*(double*)a<*(double*)b)return -1;
    else return 0;
}
int main(void){
    int n;scanf("%d",&n);
    typedef struct a{
        int xuehao;
        int ch;
        int ma;
        int eng;
        int xuan;
        double zong;
        int rank;
    }stu;
    stu t[n];
    for(int i=0;i<n;i++){
        scanf("%d%d%d%d%d",&t[i].xuehao,&t[i].ch,&t[i].ma,&t[i].eng,&t[i].xuan);
        t[i].zong=t[i].ch*1.009+t[i].ma*1.00008+t[i].eng*1.0000007+t[i].xuan;
    }
    double s[n];
    for(int i=0;i<n;i++){
        s[i]=t[i].zong;
    }
    qsort(s,n,sizeof(double),f);
    for(int i=0;i<n-1;i++){
        if(s[i]==s[i+1]){
            for(int j=i+1;j<n-1;j++){
                s[j]=s[j+1];
            }
            n--;i--;
        }
    }
    int x,y;
    scanf("%d%d",&x,&y);
    int z;
    for(int i=0;i<n;i++){
        if(t[i].xuehao==y){
            z=i;break;
        }
    }
    for(int i=0;i<n;i++){
        if(s[i]==t[z].zong){
            printf("%d\n",(n-i-1)+x);
            break;
        }
    }
    return 0;
}