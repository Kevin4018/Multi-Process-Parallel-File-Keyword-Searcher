#include <stdio.h>
#include "worker.h"

int main(void) {
    int result = count_keyword_in_file("test1.txt", "error");
    printf("count = %d\n", result);
    printf("Expected count = 4\n");
    printf("Actual count   = %d\n", result);
    
    if (result == 4) {
        printf("Test Passed!\n");
    } else {
        printf("Test Failed!\n");
    }
    
    return 0;
}