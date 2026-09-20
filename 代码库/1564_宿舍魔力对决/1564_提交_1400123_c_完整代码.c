#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_DORMS 50
#define MAX_WIZARDS 100
#define MAX_NAME_LEN 11

// 宿舍结构
typedef struct {
    char name[MAX_NAME_LEN];
} Dorm;

// 巫师结构
typedef struct {
    char name[MAX_NAME_LEN];
    int power;
    char dorm[MAX_NAME_LEN];
} Wizard;

// 字符串比较函数（用于qsort）
int compareStr(const void *a, const void *b) {
    return strcmp((char*)a, (char*)b);
}

// 巫师名字比较函数（用于qsort）
int compareWizard(const void *a, const void *b) {
    return strcmp(((Wizard*)a)->name, ((Wizard*)b)->name);
}

int main() {
    Dorm dorms[MAX_DORMS];
    Wizard wizards[MAX_WIZARDS];
    int n, m, k;
    
    // 读取宿舍信息
    scanf("%d", &n);
    for (int i = 0; i < n; i++) {
        scanf("%s", dorms[i].name);
    }
    
    // 读取巫师信息
    scanf("%d", &m);
    for (int i = 0; i < m; i++) {
        scanf("%s %d %s", wizards[i].name, &wizards[i].power, wizards[i].dorm);
    }
    
    // 读取挑战信息并处理
    scanf("%d", &k);
    for (int i = 0; i < k; i++) {
        char challenge[MAX_NAME_LEN * 2 + 3];
        scanf("%s", challenge);
        
        // 解析挑战：A->B
        char dormA[MAX_NAME_LEN], dormB[MAX_NAME_LEN];
        char *arrow = strstr(challenge, "->");
        int len = arrow - challenge;
        strncpy(dormA, challenge, len);
        dormA[len] = '\0';
        strcpy(dormB, arrow + 2);
        
        // 计算两个宿舍的魔力值总和
        int powerA = 0, powerB = 0;
        for (int j = 0; j < m; j++) {
            if (strcmp(wizards[j].dorm, dormA) == 0) {
                powerA += wizards[j].power;
            } else if (strcmp(wizards[j].dorm, dormB) == 0) {
                powerB += wizards[j].power;
            }
        }
        
        // 如果A的魔力值大于B，交换宿舍
        if (powerA > powerB) {
            for (int j = 0; j < m; j++) {
                if (strcmp(wizards[j].dorm, dormA) == 0) {
                    strcpy(wizards[j].dorm, dormB);
                } else if (strcmp(wizards[j].dorm, dormB) == 0) {
                    strcpy(wizards[j].dorm, dormA);
                }
            }
        }
    }
    
    // 输出结果
    for (int i = 0; i < n; i++) {
        printf("%s", dorms[i].name);
        
        // 收集该宿舍的所有巫师
        char wizardNames[MAX_WIZARDS][MAX_NAME_LEN];
        int count = 0;
        for (int j = 0; j < m; j++) {
            if (strcmp(wizards[j].dorm, dorms[i].name) == 0) {
                strcpy(wizardNames[count++], wizards[j].name);
            }
        }
        
        // 按字典序排序
        qsort(wizardNames, count, sizeof(wizardNames[0]), compareStr);
        
        // 输出巫师名字
        for (int j = 0; j < count; j++) {
            printf(" %s", wizardNames[j]);
        }
        printf("\n");
    }
    
    return 0;
}