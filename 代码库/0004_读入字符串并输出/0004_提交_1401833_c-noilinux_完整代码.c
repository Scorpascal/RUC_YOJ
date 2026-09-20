#include <stdio.h>
int main(void){
    char buf[128];
    scanf("%[^NULL]",buf);
    puts(buf);
    return 0;
}//你看看这个[呲牙]