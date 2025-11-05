#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>

void print_file_info(const char *filename, int show_details) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        perror(filename);
        return;
    }
    
    if (show_details) {
        // Permissions
        printf((S_ISDIR(st.st_mode)) ? "d" : "-");
        printf((st.st_mode & S_IRUSR) ? "r" : "-");
        printf((st.st_mode & S_IWUSR) ? "w" : "-");
        printf((st.st_mode & S_IXUSR) ? "x" : "-");
        printf((st.st_mode & S_IRGRP) ? "r" : "-");
        printf((st.st_mode & S_IWGRP) ? "w" : "-");
        printf((st.st_mode & S_IXGRP) ? "x" : "-");
        printf((st.st_mode & S_IROTH) ? "r" : "-");
        printf((st.st_mode & S_IWOTH) ? "w" : "-");
        printf((st.st_mode & S_IXOTH) ? "x" : "-");
        printf(" %3ld", st.st_nlink);
        
        // Owner/Group
        struct passwd *pw = getpwuid(st.st_uid);
        struct group *gr = getgrgid(st.st_gid);
        printf(" %-8s", pw ? pw->pw_name : "unknown");
        printf(" %-8s", gr ? gr->gr_name : "unknown");
        
        // Size
        printf(" %8ld", st.st_size);
        
        // Date
        char date_str[64];
        struct tm *tm_info = localtime(&st.st_mtime);
        strftime(date_str, sizeof(date_str), "%b %d %H:%M", tm_info);
        printf(" %s", date_str);
        
        printf(" %s\n", filename);
    } else {
        printf("%s  ", filename);
    }
}

int main(int argc, char *argv[]) {
    int show_details = 0;
    int show_all = 0;
    char *dir = ".";
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            show_details = 1;
        } else if (strcmp(argv[i], "-a") == 0) {
            show_all = 1;
        } else if (strcmp(argv[i], "-la") == 0 || strcmp(argv[i], "-al") == 0) {
            show_details = 1;
            show_all = 1;
        } else if (argv[i][0] != '-') {
            dir = argv[i];
        }
    }
    
    DIR *d = opendir(dir);
    if (!d) {
        perror("sandbox_ls");
        return 1;
    }
    
    struct dirent *entry;
    int count = 0;
    struct dirent **entries = NULL;
    int num_entries = 0;
    
    // Collect entries
    while ((entry = readdir(d)) != NULL) {
        if (!show_all && entry->d_name[0] == '.') {
            continue;
        }
        entries = realloc(entries, sizeof(struct dirent*) * (num_entries + 1));
        entries[num_entries] = malloc(sizeof(struct dirent));
        memcpy(entries[num_entries], entry, sizeof(struct dirent));
        num_entries++;
    }
    closedir(d);
    
    // Sort entries
    for (int i = 0; i < num_entries - 1; i++) {
        for (int j = i + 1; j < num_entries; j++) {
            if (strcmp(entries[i]->d_name, entries[j]->d_name) > 0) {
                struct dirent *temp = entries[i];
                entries[i] = entries[j];
                entries[j] = temp;
            }
        }
    }
    
    // Print entries
    char fullpath[1024];
    for (int i = 0; i < num_entries; i++) {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, entries[i]->d_name);
        print_file_info(fullpath, show_details);
        free(entries[i]);
    }
    free(entries);
    
    if (!show_details) {
        printf("\n");
    }
    
    return 0;
}

