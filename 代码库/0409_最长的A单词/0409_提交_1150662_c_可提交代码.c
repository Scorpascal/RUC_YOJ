#include <stdio.h>
#include <string.h>
#include <ctype.h>

int main(void) {
	/* 200 个单词，每个最多 50 字符，预留充足空间 */
	char line[12000];
	if (!fgets(line, sizeof(line), stdin)) {
		puts("-1");
		return 0;
	}

	int bestLen = -1;
	char best[60] = {0}; /* 单词最长 50，留余量 */
	int i = 0, n = (int)strlen(line);

	while (i < n) {
		/* 跳过前导空白 */
		while (i < n && isspace((unsigned char)line[i])) i++;
		if (i >= n) break;

		/* 记录一个单词 */
		int start = i;
		int hasA = 0;
		while (i < n && !isspace((unsigned char)line[i])) {
			char c = line[i];
			if (c == 'a' || c == 'A') hasA = 1;
			i++;
		}
		int len = i - start;

		if (hasA && len > bestLen) {
			if (len > 55) len = 55; /* 防御性截断，避免越界 */
			memcpy(best, &line[start], (size_t)len);
			best[len] = '\0';
			bestLen = len;
		}
	}

	if (bestLen == -1) puts("-1");
	else puts(best);

	return 0;
}