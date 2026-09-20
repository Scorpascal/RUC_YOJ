#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

typedef struct {
    int value;   // integer value
    char unit;   // 'C' or 'F'
} TempToken;

// Convert C to F
static double C_to_F(double c) {
    return c * 9.0 / 5.0 + 32.0;
}
// Convert F to F (identity)
static double F_to_F(double f) {
    return f;
}

// Parse all tokens of pattern: [optional sign][digits][C|F] from a text buffer
// Returns count of tokens parsed; fills out array tokens (capacity maxTokens)
static int parse_tokens(const char *buf, TempToken *tokens, int maxTokens) {
    int count = 0;
    int i = 0;
    int n = (int)strlen(buf);
    while (i < n && count < maxTokens) {
        // Skip non-sign/non-digit chars until we find start of a number or sign followed by digit
        if (buf[i] == '+' || buf[i] == '-') {
            // look ahead for digits
            int j = i + 1;
            if (j < n && isdigit((unsigned char)buf[j])) {
                // proceed
            } else {
                i++;
                continue;
            }
        } else if (!isdigit((unsigned char)buf[i])) {
            i++;
            continue;
        }

        // Parse sign
        int sign = 1;
        if (buf[i] == '+') { sign = 1; i++; }
        else if (buf[i] == '-') { sign = -1; i++; }

        // Parse digits
        if (i >= n || !isdigit((unsigned char)buf[i])) {
            // Not a valid number start, skip
            i++;
            continue;
        }
        long val = 0;
        while (i < n && isdigit((unsigned char)buf[i])) {
            val = val * 10 + (buf[i] - '0');
            i++;
        }

        // Next must be unit 'C' or 'F'
        if (i < n && (buf[i] == 'C' || buf[i] == 'F')) {
            char unit = buf[i];
            i++; // consume unit
            if (count < maxTokens) {
                tokens[count].value = (int)(sign * val);
                tokens[count].unit = unit;
                count++;
            }
        } else {
            // If there's no unit immediately after number, it's not a temperature token; skip ahead
            // Keep scanning from current position
        }
    }
    return count;
}

int main(void) {
    // Read n
    int n;
    if (scanf("%d", &n) != 1) {
        return 0;
    }
    // Consume the remainder of the first line including newline
    int ch;
    while ((ch = getchar()) != EOF && ch != '\n') { /* skip */ }

    // Read the rest of stdin into a buffer
    // Given constraints are small; allocate a reasonably large buffer
    char *buf = NULL;
    size_t cap = 8192;
    buf = (char*)malloc(cap);
    if (!buf) return 0;
    size_t len = 0;

    while ((ch = getchar()) != EOF) {
        if (len + 1 >= cap) {
            cap *= 2;
            char *nb = (char*)realloc(buf, cap);
            if (!nb) { free(buf); return 0; }
            buf = nb;
        }
        buf[len++] = (char)ch;
    }
    buf[len] = '\0';

    // Parse tokens from buffer: expecting h, l, then n temperatures
    int expected = 2 + n;
    TempToken *tokens = (TempToken*)malloc(sizeof(TempToken) * (expected + 16));
    if (!tokens) { free(buf); return 0; }

    int count = parse_tokens(buf, tokens, expected + 16);
    if (count < 2) {
        // Not enough thresholds
        free(tokens);
        free(buf);
        return 0;
    }
    // thresholds: first two tokens
    TempToken hTok = tokens[0];
    TempToken lTok = tokens[1];

    // Convert thresholds to Fahrenheit
    double hF = (hTok.unit == 'C') ? C_to_F(hTok.value) : F_to_F(hTok.value);
    double lF = (lTok.unit == 'C') ? C_to_F(lTok.value) : F_to_F(lTok.value);

    // The remaining tokens should include at least n temperatures
    // If there are more than n, only the next n after thresholds are used
    int tempsAvailable = count - 2;
    if (tempsAvailable < n) {
        // Not enough temps; but proceed with available? Spec says exactly n; here just exit
        free(tokens);
        free(buf);
        return 0;
    }

    double sumF = 0.0;
    int highDays = 0;
    int lowDays = 0;
    for (int i = 0; i < n; i++) {
        TempToken t = tokens[2 + i];
        double tF = (t.unit == 'C') ? C_to_F(t.value) : F_to_F(t.value);
        sumF += tF;
        if (tF >= hF) highDays++;
        if (tF <= lF) lowDays++;
    }

    double avgF = sumF / n;

    // Output: average with 3 decimals and 'F'
    printf("%.3fF\n", avgF);
    // Output: high and low days
    printf("%d %d\n", highDays, lowDays);

    free(tokens);
    free(buf);
    return 0;
}