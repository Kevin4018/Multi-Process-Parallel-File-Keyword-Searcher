#include <stdio.h>
#include "worker.h"

int main(void) {
    int result = count_keyword_in_file("test1.txt", "error");
    printf("count = %d\n", result);
    return 0;
}