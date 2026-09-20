#include <iostream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <memory.h>
using namespace std;

// 节点的结构体类型声明
struct node {
    int data;
    node *next;
};

// 该函数实现把 val 值插入链表最后，并返回插入后链表的头指针
node* Insert(node *head, int val) {
    node *p = NULL, *q = NULL;

    q = head;
    while (q != NULL && q->next != head)
        q = q->next;

    p = (node *)malloc(sizeof(node));
    p->data = val;

    if (q == NULL)
        head = p;
    else
        q->next = p;

    p->next = head;
    return head;
}

// 读入数据创建循环链表
node* CreateList(node *head) {
    int n, i, val;
    cin >> n;
    for (i = 0; i < n; i++) {
        cin >> val;
        head = Insert(head, val);
    }
    return head;
}

// 释放循环链表
void release(node *head) {
    node *p = head;

    if (p == NULL) // link is empty
        return;

    p = head->next;
    if (p == head) { // only have 1 node
        free(head);
        return;
    }
    // 从第2节点开始依次删除
    while (p->next != head) {
        head->next = p->next;
        free(p);
        p = head->next;
    }

    if (p != NULL)
        free(p);
}

// 给定循环链表的头指针，请返回该循环链表的节点数
int getLength(struct node* head) {
    if (head == NULL) return 0;
    int count = 1;
    struct node* p = head->next;
    while (p != NULL && p != head) {
        count++;
        p = p->next;
    }
    return count;
}

// main函数不要改动！
int main() {
    node *head = NULL;
    head = CreateList(head); // 创建链表
    cout << getLength(head) << endl; // 输出链表的节点数目
    release(head);
    return 0;
}