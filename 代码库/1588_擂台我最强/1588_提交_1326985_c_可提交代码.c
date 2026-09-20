#include<stdio.h>
#include<stdlib.h>
#include<string.h>
typedef struct{
    char name[50];
    long long int fight;
    double strength;
    int experience;
    int value;
}athlete;
int f(const void*a,const void*b){
    athlete*A=(athlete*)a;
    athlete*B=(athlete*)b;
    if(A->value!=B->value) return (B->value-A->value);
    else if(A->experience!=B->experience) return (B->experience-A->experience);
    else return strcmp(A->name,B->name);
}
int main(void){
    int n;
    scanf("%d",&n);
    athlete t[n];
    for(int i=0;i<n;i++){
        scanf("%s %lld",t[i].name,&t[i].fight);
        t[i].experience=0;
        t[i].strength=1;
        t[i].value=0;
    }
    for(int i=0,j=1;i<n&&j<n;){
        if(t[j].fight*t[j].strength>t[i].fight*t[i].strength){
            t[j].experience++;
            t[j].strength=t[j].strength*3/4;
            t[j].value+=(5-t[i].experience)>1?(5-t[i].experience):1;
            i=j++;
        }
        else{
            t[i].experience++;
            t[i].strength=t[i].strength*3/4;
            t[i].value+=5+((t[i].experience-1)<5?t[i].experience-1:5);
            j++;
        }
    }
    qsort(t,n,sizeof(athlete),f);
    for(int i=0,j=1;i<n;i++){
        if(i>0&&t[i].value<t[i-1].value)j=i+1;
        printf("%d %s %d\n",j,t[i].name,t[i].value);
    }
    return 0;
}