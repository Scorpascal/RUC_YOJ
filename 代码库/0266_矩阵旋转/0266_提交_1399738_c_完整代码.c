#include<stdio.h>
int n,m,Array[51][51];
long long Degree;     
int main(){
    scanf("%d %d",&n,&m);
    for(int i=0;i<n;i++){
        for(int j=0;j<m;j++)
            scanf("%d",&Array[i][j]);
    }  
    scanf("%lld",&Degree);
    Degree%=360;
    if(Degree<0)
    Degree+=360;
    if(Degree==0){
        for(int i=0;i<n;i++){
        for(int j=0;j<m;j++){
            printf("%d",Array[i][j]);
            if(j<m-1)
            printf(" ");
        }
            printf("\n");
    }
}
    else if(Degree==180){
        for(int i=n-1;i>=0;i--){
            for(int j=m-1;j>=0;j--){
             printf("%d",Array[i][j]);
             if(j>0)
            printf(" ");
            }
             printf("\n");
        }
    }
    else if(Degree==90){
        for(int i=0;i<m;i++){
            for(int j=n-1;j>=0;j--){
                printf("%d", Array[j][i]);
                if(j>0) printf(" ");
            }
            printf("\n");
        }
    }
    else{ // Degree == 270（或-90）
        for(int i=m-1;i>=0;i--){
            for(int j=0;j<n;j++){
                printf("%d", Array[j][i]);
                if(j < n-1) printf(" ");
            }
            printf("\n");
        }
    }
    return 0;
}