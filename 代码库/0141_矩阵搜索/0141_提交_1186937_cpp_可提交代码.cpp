#include<stdio.h>
int main(void){
    int m,n,temp=0;
    scanf("%d%d",&m,&n);
    int t[m][n],a=0,b=0;
    for(int i=0;i<m;i++){
        for(int j=0;j<n;j++){
            scanf("%d",&t[i][j]);
        }
    }
    for(int i=0;i<m;i++){
        for(int j=0;j<n;j++){
            temp=t[i][j];
            if(i-1>-1)temp+=t[i-1][j];
            if(j-1>-1)temp+=t[i][j-1];
            if(i+1<m)temp+=t[i+1][j];
            if(j+1<n)temp+=t[i][j+1];
            if(temp>a){a=temp;b=1;}
            else if(temp==a)b++;
        }
    }
    printf("%d %d\n",a,b);
    for(int i=0;i<m;i++){
        for(int j=0;j<n;j++){
            temp=t[i][j];
            if(i-1>-1)temp+=t[i-1][j];
            if(j-1>-1)temp+=t[i][j-1];
            if(i+1<m)temp+=t[i+1][j];
            if(j+1<n)temp+=t[i][j+1];
            if(temp==a)printf("%d %d\n",i,j);            
        }
    }
    return 0;
}