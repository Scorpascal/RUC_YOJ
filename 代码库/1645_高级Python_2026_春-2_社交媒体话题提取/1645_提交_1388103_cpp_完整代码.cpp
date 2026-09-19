#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_TOPIC_LEN 100
#define MAX_TOPICS   1000
#define MAX_LINE     10005

typedef struct {
    char name[MAX_TOPIC_LEN];
    int count;
} Topic;

/* 在话题数组中查找，返回下标，未找到返回 -1 */
static int find_topic(const Topic *topics, int n, const char *name) {
    for (int i = 0; i < n; ++i)
        if (strcmp(topics[i].name, name) == 0)
            return i;
    return -1;
}

/* 排序比较：频率降序，频率相同时名称字典序升序 */
static int cmp(const void *a, const void *b) {
    const Topic *ta = (const Topic *)a;
    const Topic *tb = (const Topic *)b;
    if (ta->count != tb->count)
        return tb->count - ta->count;          /* 降序 */
    return strcmp(ta->name, tb->name);        /* 升序 */
}

int main(void) {
    char line[MAX_LINE];
    if (fgets(line, sizeof(line), stdin) == NULL)
        return 0;

    Topic topics[MAX_TOPICS];
    int topic_cnt = 0;

    int i = 0;
    while (line[i] != '\0') {
        if (line[i] == '#') {
            int start = i + 1;
            int j = start;
            /* 向后扫描直到遇到空格或字符串结束 */
            while (line[j] != ' ' && line[j] != '\0')
                ++j;

            /* 只有遇到空格才算完整话题 */
            if (line[j] == ' ') {
                int len = j - start;
                if (len > 0 && len < MAX_TOPIC_LEN) {
                    char name[MAX_TOPIC_LEN];
                    strncpy(name, &line[start], len);
                    name[len] = '\0';

                    int idx = find_topic(topics, topic_cnt, name);
                    if (idx >= 0)
                        topics[idx].count++;
                    else if (topic_cnt < MAX_TOPICS) {
                        strcpy(topics[topic_cnt].name, name);
                        topics[topic_cnt].count = 1;
                        topic_cnt++;
                    }
                }
                i = j + 1;      /* 跳过空格，继续处理 */
            } else {
                /* 没有遇到空格，字符串已结束，忽略该话题 */
                i = j;          /* j 指向 '\0'，循环结束 */
            }
        } else {
            ++i;
        }
    }

    qsort(topics, topic_cnt, sizeof(Topic), cmp);

    for (int k = 0; k < topic_cnt; ++k)
        printf("%s:%d\n", topics[k].name, topics[k].count);

    return 0;
}