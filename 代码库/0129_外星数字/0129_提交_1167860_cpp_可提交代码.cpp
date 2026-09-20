#include <stdio.h>
#include <string.h>

int main(void) {
    int t;
    if (scanf("%d", &t) != 1) return 0;

    for (int case_no = 1; case_no <= t; ++case_no) {
        char alien_number[128], source_language[128], target_language[128];
        if (scanf("%s %s %s", alien_number, source_language, target_language) != 3) return 0;

        int map[256];
        for (int i = 0; i < 256; ++i) map[i] = -1;

        int source_base = (int)strlen(source_language);
        for (int i = 0; i < source_base; ++i) {
            map[(unsigned char)source_language[i]] = i;
        }

        unsigned long long value = 0;
        for (size_t i = 0; alien_number[i]; ++i) {
            value = value * source_base + (unsigned long long)map[(unsigned char)alien_number[i]];
        }

        int target_base = (int)strlen(target_language);
        char converted[256];
        int idx = 0;

        if (value == 0) {
            converted[idx++] = target_language[0];
        } else {
            while (value > 0) {
                converted[idx++] = target_language[value % target_base];
                value /= target_base;
            }
        }
        converted[idx] = '\0';

        for (int i = 0, j = idx - 1; i < j; ++i, --j) {
            char tmp = converted[i];
            converted[i] = converted[j];
            converted[j] = tmp;
        }

        printf("Case #%d: %s\n", case_no, converted);
    }

    return 0;
}