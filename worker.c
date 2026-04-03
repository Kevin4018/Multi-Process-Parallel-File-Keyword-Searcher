#include <stdio.h>
#include <string.h>
#include "worker.h"

#define LINE_SIZE 1024

int count_keyword_in_file(const char *filename, const char *keyword) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        return -1;
    }

    char line[LINE_SIZE];
    int total_count = 0;
    size_t keyword_len = strlen(keyword);

    if (keyword_len == 0) {
        fclose(fp);
        return 0;
    }

    while (fgets(line, LINE_SIZE, fp) != NULL) {
        char *pos = line;

        while ((pos = strstr(pos, keyword)) != NULL) {
            total_count++;
            pos += keyword_len;
        }
    }

    fclose(fp);
    return total_count;
}