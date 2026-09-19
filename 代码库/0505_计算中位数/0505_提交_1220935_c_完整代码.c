#include<stdio.h>
#include<string.h>
int f(const void*a,const void*b){
    return *(int*)a-*(int*)b;
}
int main(void){
    int n,m,k;
    scanf("%d%d%d",&n,&m,&k);
    int t[n][m+1];
    for(int i=0;i<n;i++){
        for(int j=0;j<m+1;j++){
            scanf("%d",*(t+i)+j);
        }
    }
    int temp[n],ave2;
    float ave=0;
    for(int i=0;i<n;i++){
        temp[i]=t[i][k];
        ave+=(float)temp[i]/n;
    }
    printf("%.1f ",ave);
    qsort(temp,n,sizeof(int),f);
    printf("%d\n",temp[n/2]);
    ave2=temp[n/2];
    for(int i=0;i<n;i++){
        temp[i]=t[i][0];
    }
    qsort(temp,n,sizeof(int),f);
    for(int i=0;i<n;i++){
        for(int j=0;j<n;j++){
            if(t[j][0]==temp[i]&&t[j][k]==ave2){
                for(int v=0;v<=m;v++){
                    printf("%d ",t[j][v]);
                }
                printf("\n");
            }
        }
    } 
    return 0;
}