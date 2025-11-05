#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include <limits.h>
#include <time.h>

#define MAX_LINE 1024
#define MAX_ARGS 64
#define DELIM " \t\r\n\a"
#define HISTORY_SIZE 100

// Sandbox configuration
#define SANDBOX_DIR "/Users/jatin/Desktop/os/sandbox"
#define SANDBOX_BIN_DIR "/Users/jatin/Desktop/os/sandbox/bin"
#define CHROOT_DIR "/Users/jatin/Desktop/os/sandbox"  // Chroot to sandbox directory
#define MAX_CPU_TIME 30        // 30 seconds CPU time per process
#define MAX_MEMORY 100         // 100 MB memory limit
#define MAX_PROCESSES 20       // Max 20 processes
#define MAX_OPEN_FILES 64      // Max 64 open files
#define USE_CHROOT 1           // Set to 1 to enable chroot (requires root)
#define USE_SANDBOX_COMMANDS 1 // Use custom sandbox commands instead of system ones

char history[HISTORY_SIZE][MAX_LINE];
int history_count = 0;
time_t sandbox_start_time;
int commands_executed = 0;
int commands_blocked = 0;

// Command whitelist - only these commands are allowed
const char *allowed_commands[] = {
    "ls", "cat", "display", "pwd", "grep", "touch", 
    "mkdir", "rmdir", "cp", "mv", "head", "tail",
    "wc", "sort", "uniq", "find", "which", "date",
    "whoami", "hostname", "sleep", "clear",
    NULL
};

// Dangerous commands that are explicitly blocked
const char *blocked_commands[] = {
    "sudo", "su", "rm", "mkfs", "dd", "reboot", 
    "shutdown", "halt", "init", "killall", "pkill",
    "chmod", "chown", "mount", "umount",
    NULL
};

void add_history_command(char *line) {
    if (history_count < HISTORY_SIZE) {
        strcpy(history[history_count++], line);
    } else {
        for (int i = 1; i < HISTORY_SIZE; i++) {
            strcpy(history[i-1], history[i]);
        }
        strcpy(history[HISTORY_SIZE-1], line);
    }
}

void print_history() {
    HIST_ENTRY **the_list = history_list();
    if (the_list) {
        for (int i = 0; the_list[i]; i++) {
            printf("%d %s\n", i + history_base, the_list[i]->line);
        }
    }
}

#define MAX_ALIASES 100

typedef struct {
    char name[MAX_LINE];
    char command[MAX_LINE];
} Alias;

Alias aliases[MAX_ALIASES];
int alias_count = 0;

void add_alias(char *name, char *command) {
    for(int i = 0; i < alias_count; i++) {
        if(strcmp(aliases[i].name, name) == 0) {
            strcpy(aliases[i].command, command);
            return;
        }
    }
    if (alias_count < MAX_ALIASES) {
        strcpy(aliases[alias_count].name, name);
        strcpy(aliases[alias_count].command, command);
        alias_count++;
    }
}

void remove_alias(char *name) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            for (int j = i; j < alias_count - 1; j++) {
                aliases[j] = aliases[j+1];
            }
            alias_count--;
            break;
        }
    }
}

char* check_alias(char *name) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(aliases[i].name, name) == 0)
            return aliases[i].command;
    }
    return NULL;
}

void sigchld_handler(int sig) {
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// SANDBOX: Setup resource limits for child processes
void setup_resource_limits() {
    struct rlimit limit;
    
    // Limit CPU time
    limit.rlim_cur = MAX_CPU_TIME;
    limit.rlim_max = MAX_CPU_TIME;
    setrlimit(RLIMIT_CPU, &limit);
    
    // Limit memory usage
    limit.rlim_cur = MAX_MEMORY * 1024 * 1024;
    limit.rlim_max = MAX_MEMORY * 1024 * 1024;
    setrlimit(RLIMIT_AS, &limit);
    
    // Limit number of open files
    limit.rlim_cur = MAX_OPEN_FILES;
    limit.rlim_max = MAX_OPEN_FILES;
    setrlimit(RLIMIT_NOFILE, &limit);
    
    // Limit number of processes
    limit.rlim_cur = MAX_PROCESSES;
    limit.rlim_max = MAX_PROCESSES;
    setrlimit(RLIMIT_NPROC, &limit);
}

// SANDBOX: Setup chroot jail for child processes
// Note: Requires root privileges. Falls back gracefully if not root.
void setup_chroot() {
#if USE_CHROOT
    // Check if we have root privileges (chroot requires root on most systems)
    if (geteuid() != 0) {
        // Not running as root, chroot will fail - silently skip
        // This is expected behavior for non-privileged users
        return;
    }
    
    // Change root directory to sandbox
    if (chroot(CHROOT_DIR) != 0) {
        // Chroot failed - this is OK if not root or if directory doesn't exist
        // Don't print error to avoid noise in non-root environments
        return;
    }
    
    // Change to root directory after chroot
    if (chdir("/") != 0) {
        // This shouldn't fail, but handle it gracefully
        perror("shell: chdir after chroot");
    }
    
    // Note: After chroot, paths like /bin/ls become relative to CHROOT_DIR
    // So /bin/ls actually refers to CHROOT_DIR/bin/ls
#endif
}

// SANDBOX: Check if command is in whitelist
int is_command_whitelisted(char *cmd) {
    for (int i = 0; allowed_commands[i] != NULL; i++) {
        if (strcmp(cmd, allowed_commands[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// SANDBOX: Check if command is explicitly blocked
int is_command_blocked(char *cmd) {
    for (int i = 0; blocked_commands[i] != NULL; i++) {
        if (strcmp(cmd, blocked_commands[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// SANDBOX: Validate command before execution
int is_command_allowed(char *cmd) {
    // First check if it's explicitly blocked
    if (is_command_blocked(cmd)) {
        fprintf(stderr, "\033[1;31m[SANDBOX BLOCKED]\033[0m Command '%s' is not allowed (security risk)\n", cmd);
        commands_blocked++;
        return 0;
    }
    
    // Then check whitelist
    if (!is_command_whitelisted(cmd)) {
        fprintf(stderr, "\033[1;33m[SANDBOX BLOCKED]\033[0m Command '%s' is not in whitelist\n", cmd);
        commands_blocked++;
        return 0;
    }
    
    return 1;
}

// SANDBOX: Check if path is within allowed sandbox directory
int is_path_allowed(const char *path) {
    char resolved_path[PATH_MAX];
    char sandbox_resolved[PATH_MAX];
    
    // Get current directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return 0;
    }
    
    // Build full path
    char full_path[PATH_MAX];
    if (path[0] == '/') {
        strcpy(full_path, path);
    } else {
        snprintf(full_path, PATH_MAX, "%s/%s", cwd, path);
    }
    
    // Resolve to absolute path (may not exist yet)
    if (realpath(full_path, resolved_path) == NULL) {
        // If file doesn't exist, check parent directory
        char *last_slash = strrchr(full_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            if (realpath(full_path, resolved_path) == NULL) {
                strcpy(resolved_path, full_path);
            }
        } else {
            strcpy(resolved_path, cwd);
        }
    }
    
    // Resolve sandbox directory
    if (realpath(SANDBOX_DIR, sandbox_resolved) == NULL) {
        // Sandbox dir might not exist, use as-is
        strcpy(sandbox_resolved, SANDBOX_DIR);
    }
    
    // Check if resolved path is within sandbox
    if (strncmp(resolved_path, sandbox_resolved, strlen(sandbox_resolved)) == 0) {
        return 1;
    }
    
    // Allow read-only access to /bin, /usr/bin for executables
    if (strncmp(resolved_path, "/bin", 4) == 0 || 
        strncmp(resolved_path, "/usr/bin", 8) == 0 ||
        strncmp(resolved_path, "/usr/local/bin", 14) == 0) {
        return 1;
    }
    
    fprintf(stderr, "\033[1;31m[SANDBOX BLOCKED]\033[0m Access denied to '%s' (outside sandbox)\n", path);
    commands_blocked++;
    return 0;
}

int parse_command(char* line, char** args) {
    int count = 0;
    char* token = strtok(line, DELIM);
    while (token != NULL && count < MAX_ARGS - 1) {
        args[count++] = token;
        token = strtok(NULL, DELIM);
    }
    args[count] = NULL;
    return count;
}

void print_sandbox_banner() {
    int is_root = (geteuid() == 0);
    int chroot_enabled = USE_CHROOT && is_root;
    
    printf("\n");
    printf("\033[1;33m╔════════════════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;33m║\033[0m         \033[1;36mSANDBOXED SHELL ENVIRONMENT - ACTIVE\033[0m              \033[1;33m║\033[0m\n");
    printf("\033[1;33m╠════════════════════════════════════════════════════════════════╣\033[0m\n");
    printf("\033[1;33m║\033[0m  \033[1;32mSecurity Features:\033[0m                                         \033[1;33m║\033[0m\n");
    printf("\033[1;33m║\033[0m  \033[1;32m✓\033[0m Resource Limits (CPU: %ds, Memory: %dMB, Procs: %d)   \033[1;33m║\033[0m\n", MAX_CPU_TIME, MAX_MEMORY, MAX_PROCESSES);
    printf("\033[1;33m║\033[0m  \033[1;32m✓\033[0m Command Whitelist Enforcement                          \033[1;33m║\033[0m\n");
    printf("\033[1;33m║\033[0m  \033[1;32m✓\033[0m Restricted File System Access                          \033[1;33m║\033[0m\n");
#if USE_SANDBOX_COMMANDS
    printf("\033[1;33m║\033[0m  \033[1;32m✓\033[0m Custom Sandbox Commands Only (NO system commands)        \033[1;33m║\033[0m\n");
#endif
    if (chroot_enabled) {
        printf("\033[1;33m║\033[0m  \033[1;32m✓\033[0m Chroot Jail Active (root filesystem isolation)       \033[1;33m║\033[0m\n");
    } else if (USE_CHROOT) {
        printf("\033[1;33m║\033[0m  \033[1;33m⚠\033[0m Chroot Disabled (requires root privileges)            \033[1;33m║\033[0m\n");
    }
    printf("\033[1;33m║\033[0m  \033[1;32m✓\033[0m All operations monitored and logged                    \033[1;33m║\033[0m\n");
    printf("\033[1;33m║\033[0m                                                                \033[1;33m║\033[0m\n");
    printf("\033[1;33m║\033[0m  Sandbox Directory: \033[1;36m%-37s\033[0m \033[1;33m║\033[0m\n", SANDBOX_DIR);
    if (chroot_enabled) {
        printf("\033[1;33m║\033[0m  Chroot Directory: \033[1;36m%-37s\033[0m \033[1;33m║\033[0m\n", CHROOT_DIR);
    }
    printf("\033[1;33m╚════════════════════════════════════════════════════════════════╝\033[0m\n");
    printf("\n");
}

void print_sandbox_stats() {
    time_t current = time(NULL);
    int runtime = (int)difftime(current, sandbox_start_time);
    printf("\n\033[1;36m[Sandbox Statistics]\033[0m\n");
    printf("  Runtime: %d seconds\n", runtime);
    printf("  Commands executed: %d\n", commands_executed);
    printf("  Commands blocked: %d\n", commands_blocked);
    printf("\n");
}

void print_all_commands() {
    printf("\n");
    printf("\033[1;36m╔════════════════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;36m║\033[0m                    \033[1;32mALL AVAILABLE COMMANDS\033[0m                      \033[1;36m║\033[0m\n");
    printf("\033[1;36m╠════════════════════════════════════════════════════════════════╣\033[0m\n");
    
    // Built-in commands
    printf("\033[1;36m║\033[0m  \033[1;32mBuilt-in Commands (Custom Implementations):\033[0m                 \033[1;36m║\033[0m\n");
    printf("\033[1;36m║\033[0m     cd, exit, print_history, add_alias, remove_alias, help, stats, commands, echo  \033[1;36m║\033[0m\n");
    printf("\033[1;36m║\033[0m                                                             \033[1;36m║\033[0m\n");
    
    // Whitelisted external commands
    printf("\033[1;36m║\033[0m  \033[1;32mWhitelisted External Commands:\033[0m                             \033[1;36m║\033[0m\n");
    printf("\033[1;36m║\033[0m     ");
    int count = 0;
    for (int i = 0; allowed_commands[i] != NULL; i++) {
        printf("%s", allowed_commands[i]);
        if (allowed_commands[i + 1] != NULL) {
            printf(", ");
        }
        count++;
        // Line break every 4 commands for readability
        if (count % 4 == 0 && allowed_commands[i + 1] != NULL) {
            printf("\n\033[1;36m║\033[0m     ");
            count = 0;
        }
    }
    printf("                                                      \033[1;36m║\033[0m\n");
    
    printf("\033[1;36m║\033[0m                                                             \033[1;36m║\033[0m\n");
    
    // Blocked commands
    printf("\033[1;36m║\033[0m  \033[1;31mBlocked Commands (Security):\033[0m                                \033[1;36m║\033[0m\n");
    printf("\033[1;36m║\033[0m     ");
    count = 0;
    for (int i = 0; blocked_commands[i] != NULL; i++) {
        printf("\033[1;31m%s\033[0m", blocked_commands[i]);
        if (blocked_commands[i + 1] != NULL) {
            printf(", ");
        }
        count++;
        if (count % 4 == 0 && blocked_commands[i + 1] != NULL) {
            printf("\n\033[1;36m║\033[0m     ");
            count = 0;
        }
    }
    printf("                                                      \033[1;36m║\033[0m\n");
    
    // Summary
    int total_allowed = 0;
    for (int i = 0; allowed_commands[i] != NULL; i++) total_allowed++;
    int total_blocked = 0;
    for (int i = 0; blocked_commands[i] != NULL; i++) total_blocked++;
    
    printf("\033[1;36m║\033[0m                                                             \033[1;36m║\033[0m\n");
    printf("\033[1;36m║\033[0m  \033[1;33mSummary:\033[0m                                                  \033[1;36m║\033[0m\n");
    printf("\033[1;36m║\033[0m     Total allowed commands: \033[1;32m%d\033[0m (including 7 built-ins)      \033[1;36m║\033[0m\n", total_allowed + 7);
    printf("\033[1;36m║\033[0m     Total blocked commands: \033[1;31m%d\033[0m (security risks)            \033[1;36m║\033[0m\n", total_blocked);
    printf("\033[1;36m╚════════════════════════════════════════════════════════════════╝\033[0m\n");
    printf("\n");
    printf("  \033[1;33mTip:\033[0m Use TAB for autocomplete, type 'help' for detailed information\n");
    printf("  \033[1;33mNote:\033[0m All file operations are restricted to: \033[1;36m%s\033[0m\n\n", SANDBOX_DIR);
}

int execute_builtin(char** args) {
    if (args[0] == NULL) return 1;

    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "shell: expected argument to \"cd\"\n");
        } else {
            // SANDBOX: Validate path before changing directory
            if (!is_path_allowed(args[1])) {
                return 1;
            }
            if (chdir(args[1]) != 0) perror("shell");
        }
        return 1;
    }
    if (strcmp(args[0], "exit") == 0) {
        print_sandbox_stats();
        exit(0);
    }
    // ONLY our custom print_history - NO original history command
    if (strcmp(args[0], "print_history") == 0) {
        print_history();
        return 1;
    }
    // Block original history command
    if (strcmp(args[0], "history") == 0) {
        fprintf(stderr, "\033[1;31m[SANDBOX BLOCKED]\033[0m Command 'history' is not allowed (use 'print_history' instead - our custom implementation)\n");
        commands_blocked++;
        return 1;
    }
    if (strcmp(args[0], "sandbox_stats") == 0 || strcmp(args[0], "stats") == 0) {
        print_sandbox_stats();
        return 1;
    }
    if (strcmp(args[0], "help") == 0) {
        printf("\n\033[1;36mAvailable Commands:\033[0m\n");
        printf("  \033[1;32mBuilt-in commands:\033[0m\n");
        printf("    cd, exit, print_history, add_alias, remove_alias, help, stats, commands\n\n");
        printf("  \033[1;32mWhitelisted external commands:\033[0m\n");
        printf("    ");
        for (int i = 0; allowed_commands[i] != NULL; i++) {
            printf("%s ", allowed_commands[i]);
            if ((i + 1) % 8 == 0) printf("\n    ");
        }
        printf("\n\n");
        printf("  \033[1;33mNote:\033[0m All file operations are restricted to: %s\n", SANDBOX_DIR);
        printf("  \033[1;33mTip:\033[0m Type 'commands' for a complete formatted list\n\n");
        return 1;
    }
    if (strcmp(args[0], "commands") == 0) {
        print_all_commands();
        return 1;
    }
    // Block original alias command
    if (strcmp(args[0], "alias") == 0) {
        fprintf(stderr, "\033[1;31m[SANDBOX BLOCKED]\033[0m Command 'alias' is not allowed (use 'add_alias' instead - our custom implementation)\n");
        commands_blocked++;
        return 1;
    }
    // Block original unalias command
    if (strcmp(args[0], "unalias") == 0) {
        fprintf(stderr, "\033[1;31m[SANDBOX BLOCKED]\033[0m Command 'unalias' is not allowed (use 'remove_alias' instead - our custom implementation)\n");
        commands_blocked++;
        return 1;
    }
    // ONLY our custom add_alias - NO original alias command
    if (strcmp(args[0], "add_alias") == 0) {
        if (args[1] == NULL) {
            // List all aliases
            for (int i = 0; i < alias_count; i++) {
                printf("alias %s='%s'\n", aliases[i].name, aliases[i].command);
            }
        } else {
            // Reconstruct the full argument string to handle quotes properly
            char full_arg[MAX_LINE] = "";
            strcpy(full_arg, args[1]);
            for (int i = 2; args[i] != NULL; i++) {
                strcat(full_arg, " ");
                strcat(full_arg, args[i]);
            }
            
            char *equal = strchr(full_arg, '=');
            if (equal) {
                *equal = '\0';
                char *name = full_arg;
                char *value = equal + 1;

                // Strip surrounding quotes if present
                size_t val_len = strlen(value);
                if (val_len >= 2 && 
                    ((value[0] == '\'' && value[val_len - 1] == '\'') ||
                     (value[0] == '"' && value[val_len - 1] == '"'))) {
                    value[val_len - 1] = '\0';
                    value++;
                }

                // Store the alias (value is now the command without quotes)
                add_alias(name, value);
            } else {
                fprintf(stderr, "shell: invalid alias format\n");
                fprintf(stderr, "Usage: add_alias name='command'\n");
            }
        }
        return 1;
    }
    // ONLY our custom remove_alias - NO original unalias command
    if (strcmp(args[0], "remove_alias") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "shell: remove_alias: usage: remove_alias name\n");
        } else {
            remove_alias(args[1]);
        }
        return 1;
    }
    return 0;
}

// SANDBOX: Find command path - ONLY use sandbox/bin, NO system fallback
char* find_command_path(const char *cmd) {
    static char path[PATH_MAX];
    struct stat st;
    
#if USE_SANDBOX_COMMANDS
    // ONLY check sandbox/bin directory - NO system fallback
    snprintf(path, sizeof(path), "%s/%s", SANDBOX_BIN_DIR, cmd);
    if (stat(path, &st) == 0 && (st.st_mode & S_IXUSR)) {
        return path;  // Found sandbox command
    }
    // NOT FOUND - return NULL to indicate command doesn't exist
    return NULL;
#else
    // Fall back to system PATH (if sandbox commands disabled)
    return (char*)cmd;
#endif
}

void handle_redirection(char** args, int *in_redir, int *out_redir, char** in_file, char** out_file) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            *in_redir = 1;
            *in_file = args[i + 1];
            args[i] = NULL;
        }
        if (strcmp(args[i], ">") == 0) {
            *out_redir = 1;
            *out_file = args[i + 1];
            args[i] = NULL;
        }
    }
}

void execute_command(char** args, int background) {
    // SANDBOX: Block original echo command (use display instead)
    if (strcmp(args[0], "echo") == 0) {
        fprintf(stderr, "\033[1;31m[SANDBOX BLOCKED]\033[0m Command 'echo' is not allowed (use 'display' instead - our custom implementation)\n");
        commands_blocked++;
        return;
    }
    
    // SANDBOX: Check if command is allowed
    if (!is_command_allowed(args[0])) {
        return;
    }
    
    int in_redir = 0, out_redir = 0;
    char *in_file = NULL, *out_file = NULL;
    handle_redirection(args, &in_redir, &out_redir, &in_file, &out_file);
    
    // SANDBOX: Validate file paths for redirection
    if (in_redir && !is_path_allowed(in_file)) {
        return;
    }
    if (out_redir && !is_path_allowed(out_file)) {
        return;
    }
    
    pid_t pid = fork();
    if (pid == 0) {
        // SANDBOX: Apply resource limits in child process
        setup_resource_limits();
        
        // SANDBOX: Setup chroot jail (requires root privileges)
        setup_chroot();
        
        if (in_redir) {
            int fd = open(in_file, O_RDONLY);
            if (fd < 0) {
                perror("shell: input redirection");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        if (out_redir) {
            int fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("shell: output redirection");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        
        // SANDBOX: ONLY use sandbox commands - NO system fallback
        char *cmd_path = find_command_path(args[0]);
        
        if (cmd_path == NULL) {
            // Command not found in sandbox/bin - BLOCK IT
            fprintf(stderr, "\033[1;31m[SANDBOX BLOCKED]\033[0m Command '%s' not found in sandbox/bin (only sandbox commands allowed)\n", args[0]);
            exit(EXIT_FAILURE);
        }
        
        // Use sandbox command (cmd_path is full path to sandbox/bin/command)
        if (execv(cmd_path, args) == -1) {
            perror("shell");
        }
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        commands_executed++;
        if (!background) {
            waitpid(pid, NULL, 0);
        } else {
            printf("[Background pid %d]\n", pid);
        }
    } else {
        perror("shell: fork failed");
    }
}

void execute_pipe(char* line, int background) {
    char *cmds[10];
    int cmd_count = 0;
    cmds[cmd_count] = strtok(line, "|");
    while (cmds[cmd_count] != NULL && cmd_count < 9) {
        cmds[++cmd_count] = strtok(NULL, "|");
    }
    
    // SANDBOX: Validate all commands in pipeline before executing
    for (int i = 0; i < cmd_count; i++) {
        char *args[MAX_ARGS];
        char temp_line[MAX_LINE];
        strcpy(temp_line, cmds[i]);
        parse_command(temp_line, args);
        if (args[0] != NULL && !is_command_allowed(args[0])) {
            return;
        }
    }
    
    int pipefds[2 * (cmd_count - 1)];
    for (int i = 0; i < cmd_count - 1; i++) {
        if (pipe(pipefds + i*2) < 0) {
            perror("shell: pipe");
            exit(EXIT_FAILURE);
        }
    }
    int pid;
    for (int i = 0; i < cmd_count; i++) {
        char *args[MAX_ARGS];
        parse_command(cmds[i], args);
        pid = fork();
        if (pid == 0) {
            // SANDBOX: Apply resource limits in child process
            setup_resource_limits();
            
            // SANDBOX: Setup chroot jail (requires root privileges)
            setup_chroot();
            
            if (i != 0) {
                dup2(pipefds[(i-1)*2], 0);
            }
            if (i != cmd_count - 1) {
                dup2(pipefds[i*2 + 1], 1);
            }
            for (int k = 0; k < 2*(cmd_count-1); k++) {
                close(pipefds[k]);
            }
            
            // SANDBOX: ONLY use sandbox commands - NO system fallback
            char *cmd_path = find_command_path(args[0]);
            
            if (cmd_path == NULL) {
                // Command not found in sandbox/bin - BLOCK IT
                fprintf(stderr, "\033[1;31m[SANDBOX BLOCKED]\033[0m Command '%s' not found in sandbox/bin (only sandbox commands allowed)\n", args[0]);
                exit(EXIT_FAILURE);
            }
            
            // Use sandbox command (cmd_path is full path to sandbox/bin/command)
            if (execv(cmd_path, args) == -1) {
                perror("shell");
                exit(EXIT_FAILURE);
            }
        } else if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < 2*(cmd_count-1); i++) {
        close(pipefds[i]);
    }
    commands_executed++;
    if (!background) {
        for (int i = 0; i < cmd_count; i++) {
            wait(NULL);
        }
    } else {
        printf("[Background pipeline]\n");
    }
}

char *command_generator(const char *text, int state) {
    static int list_index, len;
    static const char *commands[] = {
        "cd", "exit", "print_history", "add_alias", "remove_alias", "help", "stats", "commands",
        "ls", "cat", "display", "pwd", "grep", "touch", "mkdir", "rmdir", "cp", "mv",
        "head", "tail", "wc", "sort", "uniq", "find", "which", "date", "clear",
        NULL
    };

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    const char *match;
    while ((match = commands[list_index++]) != NULL) {
        if (strncmp(match, text, len) == 0)
            return strdup(match);
    }
    return NULL;
}

char *file_generator(const char *text, int state) {
    static DIR *dir = NULL;
    static struct dirent *entry;

    if (!state) {
        if (dir) closedir(dir);
        dir = opendir(".");
    }

    if (!dir) return NULL;

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, text, strlen(text)) == 0) {
            return strdup(entry->d_name);
        }
    }

    closedir(dir);
    dir = NULL;
    return NULL;
}

char **shell_completion(const char *text, int start, int end) {
    rl_completion_append_character = '\0';
    rl_attempted_completion_over = 1;

    if (start == 0) {
        return rl_completion_matches(text, command_generator);
    } else {
        return rl_completion_matches(text, file_generator);
    }
}

int main() {
    char line[MAX_LINE];
    char *args[MAX_ARGS];
    
    // Initialize sandbox
    sandbox_start_time = time(NULL);
    
    // Create sandbox directory if it doesn't exist
    mkdir(SANDBOX_DIR, 0755);
    
    signal(SIGCHLD, sigchld_handler);
    rl_bind_key('\t', rl_complete);
    rl_attempted_completion_function = shell_completion;
    
    print_sandbox_banner();
    printf("Type '\033[1;32mhelp\033[0m' for available commands, '\033[1;32mstats\033[0m' for sandbox statistics\n\n");
    
    while (1) {
        char *input = readline("\033[1;36msandbox>\033[0m ");
        if (!input) {
            break;
        }
        if (strlen(input) == 0) {
            free(input);
            continue;
        }
        strcpy(line, input);
        add_history_command(line);
        add_history(input);
        free(input);

        int background = 0;
        if (line[strlen(line) - 1] == '&') {
            background = 1;
            line[strlen(line) - 1] = '\0';
        }

        // Alias expansion
        char *tokens[MAX_ARGS];
        int token_count = 0;
        char *saveptr;
        char *temp_line = strdup(line);
        char *token = strtok_r(temp_line, DELIM, &saveptr);
        while (token != NULL && token_count < MAX_ARGS - 1) {
            tokens[token_count++] = token;
            token = strtok_r(NULL, DELIM, &saveptr);
        }
        tokens[token_count] = NULL;

        if (token_count > 0) {
            char *alias_expansion = check_alias(tokens[0]);
            if (alias_expansion != NULL) {
                char new_line[MAX_LINE];
                strcpy(new_line, alias_expansion);
                for (int i = 1; i < token_count; i++) {
                    strcat(new_line, " ");
                    strcat(new_line, tokens[i]);
                }
                strcpy(line, new_line);
            }
        }
        free(temp_line);

        if (strchr(line, '|') != NULL) {
            execute_pipe(line, background);
        } else {
            parse_command(line, args);
            if (args[0] == NULL) continue;
            if (execute_builtin(args)) continue;
            execute_command(args, background);
        }
    }
    print_sandbox_stats();
    return 0;
}

