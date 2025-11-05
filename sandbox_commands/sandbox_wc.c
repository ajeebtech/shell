#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void count_file(FILE *fp, const char *filename, int show_lines, int show_words, int show_chars) {
    int lines = 0, words = 0, chars = 0;
    int in_word = 0;
    int c;
    
    while ((c = fgetc(fp)) != EOF) {
        chars++;
        if (c == '\n') {
            lines++;
        }
        if (isspace(c)) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            words++;
        }
    }
    
    if (show_lines) printf("%8d", lines);
    if (show_words) printf("%8d", words);
    if (show_chars) printf("%8d", chars);
    if (filename) printf(" %s", filename);
    printf("\n");
}

int main(int argc, char *argv[]) {
    int show_lines = 1, show_words = 1, show_chars = 1;
    int file_count = 0;
    
    // Parse flags
    int i = 1;
    while (i < argc && argv[i][0] == '-') {
        if (strcmp(argv[i], "-l") == 0) {
            show_lines = 1;
            show_words = 0;
            show_chars = 0;
        } else if (strcmp(argv[i], "-w") == 0) {
            show_lines = 0;
            show_words = 1;
            show_chars = 0;
        } else if (strcmp(argv[i], "-c") == 0) {
            show_lines = 0;
            show_words = 0;
            show_chars = 1;
        }
        i++;
    }
    
    if (i >= argc) {
        // Read from stdin
        count_file(stdin, NULL, show_lines, show_words, show_chars);
        return 0;
    }
    
    int total_lines = 0, total_words = 0, total_chars = 0;
    
    for (; i < argc; i++) {
        FILE *fp = fopen(argv[i], "r");
        if (!fp) {
            perror(argv[i]);
            continue;
        }
        
        int lines = 0, words = 0, chars = 0;
        int in_word = 0;
        int c;
        
        while ((c = fgetc(fp)) != EOF) {
            chars++;
            if (c == '\n') lines++;
            if (isspace(c)) {
                in_word = 0;
            } else if (!in_word) {
                in_word = 1;
                words++;
            }
        }
        fclose(fp);
        
        total_lines += lines;
        total_words += words;
        total_chars += chars;
        file_count++;
        
        if (show_lines) printf("%8d", lines);
        if (show_words) printf("%8d", words);
        if (show_chars) printf("%8d", chars);
        printf(" %s\n", argv[i]);
    }
    
    if (file_count > 1) {
        if (show_lines) printf("%8d", total_lines);
        if (show_words) printf("%8d", total_words);
        if (show_chars) printf("%8d", total_chars);
        printf(" total\n");
    }
    
    return 0;
}

