#include<stdio.h>
#include<math.h>
void fab(int);
void fac(int);
void fba(int);
void fbc(int);
void fca(int);
void fcb(int);
int sum=0;
void fab(int n){
    if(n==0)return;
    fac(n-1);
    printf("[step %d] move plate %d# from %c to %c\n",1+sum++,n,'a','b');
    fcb(n-1);
}
void fac(int n){
    if(n==0)return;
    fab(n-1);
    printf("[step %d] move plate %d# from %c to %c\n",1+sum++,n,'a','c');
    fbc(n-1);

}
void fba(int n){
    if(n==0)return;
    fbc(n-1);
    printf("[step %d] move plate %d# from %c to %c\n",1+sum++,n,'b','a');
    fca(n-1);
}
void fbc(int n){
    if(n==0)return;
    fba(n-1);
    printf("[step %d] move plate %d# from %c to %c\n",1+sum++,n,'b','c');
    fac(n-1);
}
void fca(int n){
    if(n==0)return;
    fcb(n-1);
    printf("[step %d] move plate %d# from %c to %c\n",1+sum++,n,'c','a');   
    fba(n-1);
}
void fcb(int n){
    if(n==0)return;
    fca(n-1);
    printf("[step %d] move plate %d# from %c to %c\n",1+sum++,n,'c','b');
    fab(n-1);
}
int main(void){
    int n;
    scanf("%d",&n);
    fac(n);
    printf("%d\n",sum);
    return 0;
}