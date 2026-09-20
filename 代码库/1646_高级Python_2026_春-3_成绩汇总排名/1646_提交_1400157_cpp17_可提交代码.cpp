#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_STUDENTS 1000
#define MAX_LINE     256

typedef struct {
    char id[20];
    char name[50];
    double total;
    int count;
} Student;

/* 按平均分降序，平均分相同时按学号升序 */
static int cmp(const void *a, const void *b) {
    const Student *sa = (const Student *)a;
    const Student *sb = (const Student *)b;
    double avg_a = sa->total / sa->count;
    double avg_b = sb->total / sb->count;
    if (avg_a > avg_b) return -1;
    if (avg_a < avg_b) return  1;
    /* 平均分相同，学号字典序升序 */
    return strcmp(sa->id, sb->id);
}

int main(void) {
    Student stu[MAX_STUDENTS];
    int stu_cnt = 0;
    int N;
    char line[MAX_LINE];

    if (fgets(line, sizeof(line), stdin) == NULL) return 1;
    N = atoi(line);

    for (int i = 0; i < N; ++i) {
        if (fgets(line, sizeof(line), stdin) == NULL) break;

        char id[20], name[50], course[50];
        double score;
        /* 解析学号 姓名 课程名 分数 */
        if (sscanf(line, "%s %s %s %lf", id, name, course, &score) != 4)
            continue;

        /* 查找该学生是否存在 */
        int found = -1;
        for (int j = 0; j < stu_cnt; ++j) {
            if (strcmp(stu[j].id, id) == 0) {
                found = j;
                break;
            }
        }

        if (found >= 0) {
            stu[found].total += score;
            stu[found].count++;
        } else {
            if (stu_cnt >= MAX_STUDENTS) {
                fprintf(stderr, "too many students\n");
                break;
            }
            strcpy(stu[stu_cnt].id, id);
            strcpy(stu[stu_cnt].name, name);
            stu[stu_cnt].total = score;
            stu[stu_cnt].count = 1;
            stu_cnt++;
        }
    }

    /* 排序 */
    qsort(stu, stu_cnt, sizeof(Student), cmp);

    /* 输出 */
    for (int i = 0; i < stu_cnt; ++i) {
        double avg = stu[i].total / stu[i].count;
        printf("%s %s %.2f\n", stu[i].id, stu[i].name, avg);
    }

    return 0;
}