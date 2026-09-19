#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <cstring>
using namespace std;

int replaceStr(char A[ ], const char B[ ], const char C[ ])
{
//补充代码，完成此函数
____qcodep____
}



int main()
{
    char str1[110], str2[110], str3[110];
    int nRes;

    cin.getline(str1, 100);
    cin.getline(str2, 100);
    cin.getline(str3, 100);

    nRes = replaceStr(str1, str2, str3);
    printf("%d\n", nRes);
    printf("%s\n", str1);
    return 0;

}

