#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Custom echo implementation - Sandboxed Shell
// Supports: -n (no newline), -e (escape sequences), -E (no escape sequences)

void print_escaped(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\\' && str[i + 1] != '\0') {
            switch (str[i + 1]) {
                case 'n':  putchar('\n'); i++; break;
                case 't':  putchar('\t'); i++; break;
                case 'r':  putchar('\r'); i++; break;
                case '\\': putchar('\\'); i++; break;
                case 'a':  putchar('\a'); i++; break;
                case 'b':  putchar('\b'); i++; break;
                case 'v':  putchar('\v'); i++; break;
                case 'f':  putchar('\f'); i++; break;
                default:
                    putchar(str[i]);
                    break;
            }
        } else {
            putchar(str[i]);
        }
    }
}

int main(int argc, char *argv[]) {
    int no_newline = 0;
    int interpret_escapes = 0;
    int start = 1;
    
    // Parse flags
    while (start < argc && argv[start][0] == '-') {
        if (strcmp(argv[start], "-n") == 0) {
            no_newline = 1;
            start++;
        } else if (strcmp(argv[start], "-e") == 0) {
            interpret_escapes = 1;
            start++;
        } else if (strcmp(argv[start], "-E") == 0) {
            interpret_escapes = 0;
            start++;
        } else if (strcmp(argv[start], "-ne") == 0 || strcmp(argv[start], "-en") == 0) {
            no_newline = 1;
            interpret_escapes = 1;
            start++;
        } else {
            break;  // Unknown flag, treat as argument
        }
    }
    
    // Print arguments
    for (int i = start; i < argc; i++) {
        if (interpret_escapes) {
            print_escaped(argv[i]);
        } else {
            printf("%s", argv[i]);
        }
        if (i < argc - 1) {
            printf(" ");
        }
    }
    
    // Print newline unless -n flag
    if (!no_newline) {
        printf("\n");
    }
    
    return 0;
}

