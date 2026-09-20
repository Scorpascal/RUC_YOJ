#include <stdio.h>
int main() {
int n, i;
int a, b, c, d, e, f;
unsigned char cnt1, cnt2, cnt3, cnt4, cnt5, cnt6;
short score;
int sum;
int flag;
scanf("%d", &n);
 i = 1;
 flag = 0;
 sum = 0;
 while (!flag && i<=n) {
 scanf("%d%d%d%d%d%d", &a, &b, &c, &d, &e, &f);
 cnt1 = (a==1) + (b==1) + (c==1) + (d==1) + (e==1) + (f==1);
 cnt2 = (a==2) + (b==2) + (c==2) + (d==2) + (e==2) + (f==2);
 cnt3 = (a==3) + (b==3) + (c==3) + (d==3) + (e==3) + (f==3);
 cnt4 = (a==4) + (b==4) + (c==4) + (d==4) + (e==4) + (f==4);
 cnt5 = (a==5) + (b==5) + (c==5) + (d==5) + (e==5) + (f==5);
 cnt6 = (a==6) + (b==6) + (c==6) + (d==6) + (e==6) + (f==6);
if (cnt4 == 4 && cnt1 == 2) score = 2048;
 else if (cnt4 == 6) score = 1024;
 else if (cnt1 == 6) score = 512;
 else if (cnt2 == 6) score = 256;
 else if (cnt4 == 5) score = 128;
 else if (cnt2 == 5) score = 64;
 else if (cnt4 == 4) score = 32;
 else if (cnt1==1 && cnt2==1 && cnt3==1 && cnt4==1 && cnt5==1 && cnt6==1) score = 16;
 else if (cnt4 == 3) score = 8;
 else if (cnt2 == 4) score = 4;
 else if (cnt4 == 2) score = 2;
 else if (cnt4 == 1) score = 1;
 else score = 0;
 if (score == 0) flag = 1;
 else sum += score;
 i++;
}
printf("%x\n", sum);
}