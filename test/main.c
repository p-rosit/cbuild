#include <stdio.h>
#include "test.h"

int main() {
    int a = 5;
    int b = 3;

    printf("||(%d, %d)|| = %lf\n", a, b, func(a, b));

    return 0;
}
