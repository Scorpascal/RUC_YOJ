#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <cstring>
using namespace std;

int replaceStr(char A[ ], const char B[ ], const char C[ ])
{
//补充代码，完成此函数
 // 若B为空串，按“找不到”处理：追加C
    if (B == NULL || B[0] == '\0') {
        strcat(A, C);
        return 0;
    }

    char* pos = strstr(A, B);
    if (pos != NULL) {
        int prefixLen = (int)(pos - A);
        int bLen = (int)strlen(B);

        char tmp[300] = {0};

        strncpy(tmp, A, prefixLen);
        tmp[prefixLen] = '\0';

        strcat(tmp, C);
        strcat(tmp, pos + bLen);

        strcpy(A, tmp);
        return 1;
    } else {
        strcat(A, C);
        return 0;
    }
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

