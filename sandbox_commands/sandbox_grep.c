#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void grep_file(FILE *fp, const char *pattern, const char *filename, int show_filename) {
    char line[4096];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        if (strstr(line, pattern) != NULL) {
            if (show_filename) {
                printf("%s:", filename);
            }
            printf("%s", line);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "sandbox_grep: missing pattern\n");
        return 1;
    }
    
    char *pattern = argv[1];
    int show_filename = (argc > 2);
    
    if (argc == 2) {
        // Read from stdin
        grep_file(stdin, pattern, "(stdin)", 0);
        return 0;
    }
    
    for (int i = 2; i < argc; i++) {
        FILE *fp = fopen(argv[i], "r");
        if (!fp) {
            perror(argv[i]);
            continue;
        }
        grep_file(fp, pattern, argv[i], show_filename);
        fclose(fp);
    }
    
    return 0;
}

