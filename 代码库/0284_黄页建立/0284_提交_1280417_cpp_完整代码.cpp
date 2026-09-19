#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct YellowPage {
    char name[21];    // 姓名，长度不超过20，+1存放终止符
    int telNum;       // 电话号码
    struct YellowPage *next;
} YellowPage;

static YellowPage* insert_sorted(YellowPage* head, YellowPage* node) {
    // 按 name 字典序插入，稳定：同名插入到已有同名的最后
    if (!head) {
        node->next = NULL;
        return node;
    }
    // 若应插到头部（严格小于头部 name）
    if (strcmp(node->name, head->name) < 0) {
        node->next = head;
        return node;
    }
    YellowPage* prev = NULL;
    YellowPage* cur = head;
    while (cur) {
        int cmp = strcmp(node->name, cur->name);
        if (cmp < 0) {
            break;
        }
        // 对于同名（cmp == 0），继续向后，以保证稳定性
        if (cmp == 0) {
            prev = cur;
            cur = cur->next;
            continue;
        }
        // cmp > 0，继续前进
        prev = cur;
        cur = cur->next;
    }
    // 插入到 prev 与 cur 之间
    if (prev) {
        prev->next = node;
    }
    node->next = cur;
    return head;
}

YellowPage* create() {
    int N;
    if (scanf("%d", &N) != 1) {
        return NULL;
    }
    YellowPage* head = NULL;
    for (int i = 0; i < N; ++i) {
        char name[64];
        int tel;
        if (scanf("%20s %d", name, &tel) != 2) {
            // 输入异常，释放已分配的链表并返回
            YellowPage* p = head;
            while (p) {
                YellowPage* t = p->next;
                free(p);
                p = t;
            }
            return NULL;
        }
        YellowPage* node = (YellowPage*)malloc(sizeof(YellowPage));
        if (!node) {
            // 内存分配失败，释放并返回
            YellowPage* p = head;
            while (p) {
                YellowPage* t = p->next;
                free(p);
                p = t;
            }
            return NULL;
        }
        strncpy(node->name, name, sizeof(node->name) - 1);
        node->name[sizeof(node->name) - 1] = '\0';
        node->telNum = tel;
        node->next = NULL;
        head = insert_sorted(head, node);
    }
    return head;
} 

void realse(YellowPage *head)
{
    YellowPage *p = head, *pre;
    while (p != NULL)
    {
        pre = p, p = p->next;
        pre->next = NULL;
        free(pre);
    }
}

void display(YellowPage *head)
{
    YellowPage *node = head;
    printf("display data \n");
    while (node != NULL)
    {
        printf("%s %d \n", node->name, node->telNum);
        node = node->next;
    }
    realse(head);
}
int main()
{
    display(create());
    return 0;
}