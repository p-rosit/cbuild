#include <stdio.h>
#include "directory/file.h"
#include "directory/dist.h"

int main() {
    int a = 5;
    int b = 3;

    printf("||(%d, %d)|| = %lf\n", a, b, func(a, b));
    printf("||(%d, %d)|| = %lf\n", a, b, dist(a, b));

    return 0;
}
