#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <utime.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "sandbox_touch: missing file operand\n");
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        FILE *fp = fopen(argv[i], "a");
        if (fp) {
            fclose(fp);
            // Update timestamp
            utime(argv[i], NULL);
        } else {
            perror("sandbox_touch");
            return 1;
        }
    }
    
    return 0;
}

