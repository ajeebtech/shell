#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "sandbox_mkdir: missing operand\n");
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        if (mkdir(argv[i], 0755) != 0) {
            perror("sandbox_mkdir");
            return 1;
        }
    }
    
    return 0;
}

