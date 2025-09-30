#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>

#define MAX_LINE 1024
#define MAX_ARGS 64
#define DELIM " \t\r\n\a"
#define HISTORY_SIZE 100

char history[HISTORY_SIZE][MAX_LINE];
int history_count = 0;

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

int execute_builtin(char** args) {
    if (args[0] == NULL) return 1;

    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "shell: expected argument to \"cd\"\n");
        } else {
            if (chdir(args[1]) != 0) perror("shell");
        }
        return 1;
    }
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }
    if (strcmp(args[0], "history") == 0 || strcmp(args[0], "print_history") == 0) {
    print_history();
    return 1;
}
    if (strcmp(args[0], "alias") == 0) {
        if (args[1] == NULL) {
            for (int i = 0; i < alias_count; i++) {
                printf("alias %s='%s'\n", aliases[i].name, aliases[i].command);
            }
        } else {
            char *equal = strchr(args[1], '=');
            if (equal) {
                *equal = '\0';
                char *name = args[1];
                char *value = equal + 1;

                // Strip surrounding quotes if present
                size_t val_len = strlen(value);
                if ((value[0] == '\'' && value[val_len - 1] == '\'') ||
                    (value[0] == '"' && value[val_len - 1] == '"')) {
                    value[val_len - 1] = '\0';
                    value++;
                }

                // Rebuild full alias command including additional args after '='
                char full_command[MAX_LINE];
                strcpy(full_command, value);
                for (int i = 2; args[i] != NULL; i++) {
                    strcat(full_command, " ");
                    strcat(full_command, args[i]);
                }
                add_alias(name, full_command);
            } else {
                fprintf(stderr, "shell: invalid alias format\n");
            }
        }
        return 1;
    }
    if (strcmp(args[0], "unalias") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "shell: unalias: usage: unalias name\n");
        } else {
            remove_alias(args[1]);
        }
        return 1;
    }
    return 0;
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
    int in_redir = 0, out_redir = 0;
    char *in_file = NULL, *out_file = NULL;
    handle_redirection(args, &in_redir, &out_redir, &in_file, &out_file);
    pid_t pid = fork();
    if (pid == 0) {
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
        if (execvp(args[0], args) == -1) {
            perror("shell");
        }
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
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
            if (i != 0) {
                dup2(pipefds[(i-1)*2], 0);
            }
            if (i != cmd_count - 1) {
                dup2(pipefds[i*2 + 1], 1);
            }
            for (int k = 0; k < 2*(cmd_count-1); k++) {
                close(pipefds[k]);
            }
            if (execvp(args[0], args) == -1) {
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
        "cd", "exit", "history", "alias", "unalias",
        "ls", "cat", "echo", "pwd", "grep", "touch", "rm", "mkdir", "rmdir", "cp", "mv",
        "head", "tail", "more", "less", "find", "which", "clear",
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
    static int done = 0;

    if (!state) {
        if (dir) closedir(dir);
        dir = opendir(".");
        done = 0;
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
        // first token → command completion
        return rl_completion_matches(text, command_generator);
    } else {
        // subsequent tokens → file completion
        return rl_completion_matches(text, file_generator);
    }
}

int main() {
    char line[MAX_LINE];
    char *args[MAX_ARGS];
    signal(SIGCHLD, sigchld_handler);
    rl_bind_key('\t', rl_complete);
    rl_attempted_completion_function = shell_completion;
    while (1) {
        char *input = readline("shell> ");
        if (!input) {
            break;
        }
        if (strlen(input) == 0) {
            free(input);
            continue;
        }
        strcpy(line, input);
        add_history_command(line);
        add_history(input);  // also store in readline's history
        free(input);

        int background = 0;
        if (line[strlen(line) - 1] == '&') {
            background = 1;
            line[strlen(line) - 1] = '\0';
        }

        // Alias expansion for first token only:
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
    return 0;
}
