#include <stdio.h>

static int is_leap(int y) {
    return (y%4==0 && y%100!=0) || (y%400==0);
}

static int days_in_year(int y) {
    return is_leap(y) ? 366 : 365;
}

// 0=Sunday, 1=Monday, ... 6=Saturday
static int weekday_jan1(int y) {
    int w = 1; // 1900-01-01 is Monday -> 1
    for (int yy = 1900; yy < y; ++yy) {
        w = (w + days_in_year(yy)) % 7;
    }
    return w;
}

static void doy_to_date(int y, int doy, int *m, int *d) {
    static const int mdays_norm[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int mdays[12];
    for (int i=0;i<12;++i) mdays[i]=mdays_norm[i];
    if (is_leap(y)) mdays[1]=29;
    int mm=1;
    for (int i=0;i<12;++i) {
        if (doy > mdays[i]) {
            doy -= mdays[i];
            mm++;
        } else {
            *m = mm;
            *d = doy;
            return;
        }
    }
    // fallback (shouldn't happen)
    *m = 12; *d = 31;
}

int main(void) {
    int n;
    if (scanf("%d", &n) != 1) return 0;
    for (int i=0;i<n;++i) {
        int year, weekno;
        if (scanf("%d %d", &year, &weekno) != 2) { printf("-1\n"); continue; }
        if (year < 1980 || year > 2040 || weekno < 1 || weekno > 54) { printf("-1\n"); continue; }

        int w = weekday_jan1(year);          // 0..6
        int last = days_in_year(year);

        int start_doy = 0;
        int end_doy = 0;
        int tuesday_doy = 0;
        if (weekno == 1) {
            start_doy = 1;
            end_doy = 1 + (6 - w);           // up to Saturday of first partial week
            if (end_doy > last) end_doy = last;
            // first Tuesday >= Jan1
            int delta = (2 - w + 7) % 7;     // days from Jan1 to next Tuesday (including same day)
            tuesday_doy = 1 + delta;
            if (tuesday_doy < start_doy || tuesday_doy > end_doy) {
                printf("-1\n");
                continue;
            }
        } else {
            // week 2 starts on the first Sunday after the first partial week
            int first_sunday_doy = 1 + (7 - w); // if w=0 (Sunday), this gives 8 (next Sunday), correct per spec (week1 is partial starting Jan1)
            start_doy = first_sunday_doy + (weekno - 2) * 7;
            end_doy = start_doy + 6;
            if (start_doy > last) { printf("-1\n"); continue; }
            if (end_doy > last) end_doy = last;
            tuesday_doy = start_doy + 2;
            if (tuesday_doy < start_doy || tuesday_doy > end_doy || tuesday_doy > last) {
                printf("-1\n");
                continue;
            }
        }

        int m, d;
        doy_to_date(year, tuesday_doy, &m, &d);
        printf("%04d%02d%02d\n", year, m, d);
    }
    return 0;
}