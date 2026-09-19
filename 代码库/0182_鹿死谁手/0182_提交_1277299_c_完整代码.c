#include <stdio.h>

int main() {
    int n;
    if (scanf("%d", &n) != 1) return 0;

    const char persons[8] = {'A','B','C','D','E','F','G','H'};
    int solutions = 0;
    char answer = '?';

    for (int i = 0; i < 8; ++i) {
        char s = persons[i];
        int cnt = 0;

        // A: H or F
        cnt += (s == 'H' || s == 'F');
        // B: B
        cnt += (s == 'B');
        // C: G
        cnt += (s == 'G');
        // D: not B
        cnt += (s != 'B');
        // E: A guessed wrong => !(H or F)
        cnt += !(s == 'H' || s == 'F');
        // F: not F and not H
        cnt += (s != 'F' && s != 'H');
        // G: not C
        cnt += (s != 'C');
        // H: agree with A => (H or F)
        cnt += (s == 'H' || s == 'F');

        if (cnt == n) {
            solutions++;
            answer = s;
        }
    }

    if (solutions == 1) {
        printf("%c\n", answer);
    } else {
        printf("DONTKNOW\n");
    }
    return 0;
}