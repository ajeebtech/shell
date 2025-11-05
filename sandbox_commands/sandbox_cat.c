#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc == 1) {
        // Read from stdin
        int c;
        while ((c = getchar()) != EOF) {
            putchar(c);
        }
        return 0;
    }
    
    for (int i = 1; i < argc; i++) {
        FILE *fp = fopen(argv[i], "r");
        if (!fp) {
            fprintf(stderr, "sandbox_cat: %s: No such file or directory\n", argv[i]);
            continue;
        }
        
        int c;
        while ((c = fgetc(fp)) != EOF) {
            putchar(c);
        }
        fclose(fp);
    }
    
    return 0;
}

