#include <stdio.h>
int main() {
    int a, b, c, d;
    scanf("%d %d %d %d", &a, &b, &c, &d);
    if (a == b && b == c && c == d) {
        printf("Fish At Constant Depth\n");
    } else if (a < b && b < c && c < d) {
        printf("Fish Rising\n");
    } else if (a > b && b > c && c > d) {
        printf("Fish Diving\n");
    } else {
        printf("No Fish\n");
    }
    return 0;
}