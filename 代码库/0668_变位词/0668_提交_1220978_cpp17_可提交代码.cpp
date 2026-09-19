#include<stdio.h>
#include<string.h>
int main(void){
    int n;
    scanf("%d",&n);
    char t[n][26];
    for(int i=0;i<n;i++){
        scanf("%s",*(t+i));
    }
    int p[n][26];
    for(int i=0;i<n;i++){
        memset(*(p+i),0,26*sizeof(int));
    }
    int sum=n,b=0,temp1,temp2;
    for(int i=0;i<n;i++){
        int temp[26]={0};
        for(int j=0;j<strlen(t[i]);j++){
            temp[t[i][j]-'a']++;
        }
        for(int k=0;k<=b;k++){
            temp1=0;
            for(int l=0;l<26;l++){
                if(p[k][l]==temp[l])temp1++;
            }
            if(temp1==26){
                sum--;break;//及时break,防止与p[b](全0行)再次进行比较
            }
            else{
                if(k==b){
                for(int m=0;m<26;m++){
                    p[b][m]=temp[m];
                }
                b++;
                break;//及时break防止k继续小于b,陷入死循环
                }
            
            }
        }
    }
    printf("%d\n",sum);
    return 0;
}