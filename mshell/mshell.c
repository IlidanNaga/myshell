#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <limits.h>

struct command {

    char **data;
    int status[15];
};

int background_pid[256] = {0};
int background_amount = 0;

void mshell_init(void);                              // function which makes everything work
int mshell_getlex(char **buffer, int * status);      // returning lexeme type, in **buffer - string of inner data
char **mshell_buildarr(int *status, int **build);                 // building array from lexemes we got
int mshell_execute(char **data, int *status);        // working with data we got in _getwords, must be splitted into xtra's

int mshell_forks(char **data);                       // working with data - fork way
int mshell_background(char **data, int *status);     // one more idea of implementation
void mshell_background_help();                       // function used 4 printing status of programs

int mshell_cd(char **data);                          // working with data - got cd
int mshell_exit(char **data);                        // working with data - got exit
int mshell_help(char **data);                        // printing all builtins i made
int mshell_builtins_am();                            // returning amount of builtin functions we realise

int one_more_execute(char **data, int *status, int *build); //pending
int redirect_out(char **data, char **target, int *status);   //redirecting stdout into target
int redirect_in(char **data, char **source, int *status);    //redirecting stdin into source
int mshell_factory(char ***data, int commands_amount);       //factory... that's all i guess

char *builtin_str[] = {
        "cd", "exit","help"
};
int (*builtin_func[]) (char **) = {
        &mshell_cd, &mshell_exit, &mshell_help
};
enum lexemes {NUL, SC, IN, OUT, ADD, PIPE, OR, BACK, AND, LP, RP, END, ERROR, WORD, EndFile};


// "main" - literaly does nothing, just redirects input if we get file in
int main(int argc, char **argv) {

    signal(SIGINT, SIG_IGN);

    if (argc == 2) {
        int fd = open(argv[1], O_RDONLY);
        if (fd != -1) {
            dup2(fd, 0);
            close(fd);
        }
    }

    mshell_init();

    return 0;

}


#define we_add_len 20                   // const used to increase buffer size
void mshell_init(void) {
    /* hub, controls the work of the program*/

    char **data;
    int *build = NULL;

    int status[4] = {0, 0, 0, 0};

    // status[2] is amount of words in command
    // status[3] is length of our build-like data implementation
    // build - like implementation lays in int *build

    do {

        char path[PATH_MAX];

        getcwd(path, PATH_MAX);

        printf("%s: ", path);

        data = mshell_buildarr(status, &build);
        status[1] = one_more_execute(data, status, build);

        data = NULL;

        mshell_background_help();
    } while (status[1]);

}

// getting lexemes
int mshell_getlex(char **buffer, int *status) {
    /* getting lexemes \ words */

    char buff;
    char *local_buffer = NULL;
    int local_len = 0;

    (*buffer) = NULL;

    while(1) {

        buff = getchar();

        switch (buff) {

            case '|':

                buff = getchar();

                if (buff == '|')
                    return OR;

                ungetc(buff, stdin);
                return PIPE;
            case ';':

                return END;
            case '<':

                return IN;
            case '>':

                buff = getchar();

                if (buff == '>')
                    return ADD;
                else {
                    ungetc(buff, stdin);
                    return OUT;
                }
            case '&':

                buff = getchar();
                if (buff == '&')
                    return AND;
                else {
                    ungetc(buff, stdin);
                    return BACK;
                }
                break;
            case '(':
                return LP;
                break;
            case ')':
                return RP;
                break;
            case EOF:

                return EndFile;
                break;
            case '#':
                break;
            case ' ':
                break;
            case '\n':

                return END;
                break;
            case '"':

                buff = getchar();

                while (buff != '"') {

                    if (buff == '\n' || buff == EOF)
                        return ERROR;

                    if (local_len % we_add_len == 0)
                        local_buffer = (char *) realloc(local_buffer, (local_len + we_add_len) * sizeof(char));

                    local_buffer[local_len] = buff;
                    local_len++;

                    buff = getchar();
                }

                (*buffer) = local_buffer;
                return WORD;
                break;
            default:

                while (buff != ' ' && buff != '\n' && buff != EOF) {

                    if (buff == '|' || buff == '>' || buff == '&' || buff == ';' || buff == '<')
                        break;

                    if (local_len % we_add_len == 0)
                        local_buffer = (char *) realloc(local_buffer, (local_len + we_add_len) * sizeof(char));

                    local_buffer[local_len] = buff;
                    local_len++;

                    buff = getchar();
                }

                (*buffer) = local_buffer;
                ungetc(buff, stdin);
                return WORD;
                break;
        }
    }
}
// building string array
char **mshell_buildarr(int *status, int **build) {

    char **data = NULL;
    char *buffer;
    int words_num = 0;
    int stat;
    int *laziness = NULL;
    int info_len = 0;

    do {

        buffer = NULL;

        stat = mshell_getlex(&buffer, status);

        laziness = (int *) realloc(laziness, (info_len + 1) * sizeof(int));
        laziness[info_len] = stat;
        info_len ++;

        switch(stat) {

            case IN:

                data = (char **) realloc(data,(words_num + 1) * sizeof(char*));
                data[words_num] = "<";
                words_num ++;

                break;

            case OUT:

                data = (char **) realloc(data,(words_num + 1) * sizeof(char*));
                data[words_num] = ">";
                words_num ++;

                break;

            case PIPE:

                data = (char **) realloc(data,(words_num + 1) * sizeof(char*));
                data[words_num] = "|";
                words_num ++;

                break;

            case OR:

                data = (char **) realloc(data,(words_num + 1) * sizeof(char*));
                data[words_num] = "||";
                words_num ++;

                break;

            case BACK:

                data = (char **) realloc(data,(words_num + 1) * sizeof(char*));
                data[words_num] = "&";
                words_num ++;

                break;

            case AND:

                data = (char **) realloc(data,(words_num + 1) * sizeof(char*));
                data[words_num] = "&&";
                words_num ++;

                break;

            case LP:

                data = (char **) realloc(data,(words_num + 1) * sizeof(char*));
                data[words_num] = "(";
                words_num ++;

                break;

            case RP:

                data = (char **) realloc(data,(words_num + 1) * sizeof(char*));
                data[words_num] = ")";
                words_num ++;

                break;

            case END:
                break;

            case ERROR:

                printf("Wrong word buildage\n");
                break;

            case WORD:

                data = (char **) realloc(data,(words_num + 1) * sizeof(char*));
                data[words_num] = buffer;
                words_num ++;

                break;

            case EndFile:
                stat = END;
                break;

            default:
                break;
        }

    } while (stat != END && stat != ERROR);

    status[2] = words_num;
    status[3] = info_len;

    (*build) = laziness;

    return data;

}
// one more hub function, transmits data
int one_more_execute(char **data, int *status, int *build) {

    if (data == NULL)
        return 1;


    int exit_flag = 1;
    int i = 0, k;

    char ***command_storage = NULL;
    int command_amount = 0;

    int command_associations[1024] = {0};

    do {

        char **command = NULL;
        int command_len = 0;

        switch(build[i]) {

            case EndFile:

                break;
            case END:

                break;

            case ERROR:

                exit_flag = 0;

                break;

            case WORD:

                command = (char**) malloc(sizeof(char*));

                command[0] = data[i];
                command_len ++;

                while (build[i + 1] == WORD) {

                    command = (char**) realloc(command, (command_len + 1) * sizeof(char*));
                    command[command_len] = data[i + 1];
                    i ++;
                    command_len ++;
                }

                command = (char **) realloc(command, (command_len + 1) * sizeof(char*));
                command[command_len] = NULL;

                command_storage = (char ***) realloc(command_storage, (command_amount + 1) * sizeof(char **));
                command_storage[command_amount] = command;
                command_associations[command_amount] = WORD;
                command_amount ++;

                break;

            case BACK:

                if (i != status[3] - 2  || command_amount == 0) {
                    exit_flag = 0;
                    printf("Error - command composition\n");
                    break;
                }
                command_associations[command_amount - 1] = BACK;
                break;

            case IN:

                if (command_amount == 0) {
                    exit_flag = 0;
                    printf("Error - command composition\n");
                    break;
                }
                command_associations[command_amount - 1] = IN;

                break;

            case OUT:

                if (command_amount == 0) {
                    exit_flag = 0;
                    printf("Error - command composition\n");
                    break;
                }
                command_associations[command_amount - 1] = OUT;
                break;

            case PIPE:

                if (command_amount == 0) {
                    exit_flag = 0;
                    printf("Error - command composition\n");
                    break;
                }
                command_associations[command_amount - 1] = PIPE;
                break;


            default:
                break;
        }

        i++;
    } while (i < status[3] && exit_flag);

    if (!exit_flag)
        return 1;

    for (i = 0; i < 1024; i++) {
        switch(command_associations[i]) {

            case WORD:

                for (k = 0; k < mshell_builtins_am(); k++) {
                    if (strcmp(command_storage[i][0], builtin_str[k]) == 0) {
                        return (*builtin_func[k])(command_storage[i]);
                    }
                }

                return mshell_forks(command_storage[i]);

                break;

            case BACK:

                return mshell_background(command_storage[i], status);

                break;

            case OUT:

                if (command_associations[i + 1] != WORD) {
                    printf("Error - command composition\n");
                    return 1;
                }
                return redirect_out(command_storage[i], command_storage[i + 1], status);

                break;

            case IN:

                if (command_associations[i + 1] != WORD) {
                    printf("Error - command composition\n");
                    return 1;
                }
                return redirect_in(command_storage[i], command_storage[i + 1], status);

                break;

            case PIPE:

                if (command_associations[i + 1] != PIPE && command_associations[i + 1] != WORD) {
                    printf("Error - command composition\n");
                    return 1;
                }
                return mshell_factory(command_storage, command_amount);

                break;
            default:
                break;
        }


        i ++;
    }

    return 1;
}

//redirecting in and out
int redirect_out(char **data, char **target, int *status) {

    int pid, wpid, stat;

    if (target[0] == NULL) {
        printf("Wrong input\n");
        return 1;
    }

    if (target[1] != NULL) {
        printf("Too big input\n");
        return 1;
    }


    if ((pid = fork()) == 0) {

        int fd = open(target[0], O_WRONLY);
        dup2(fd, 1);
        close(fd);

        execvp(data[0], data);
        perror("redirect out - exec");

        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("redirect out - fork");
    } else {

        do {

            wpid = waitpid(pid, &stat, WUNTRACED);
        } while (!WIFEXITED(stat) && !WIFSIGNALED(stat));
    }

    return 1;
}
int redirect_in(char **data, char **source, int *status) {

    int pid, wpid, stat;

    if (source[0] == NULL) {
        printf("Wrong input\n");
        return 1;
    }

    if (source[1] != NULL) {
        printf("Too big input\n");
        return 1;
    }


    if ((pid = fork()) == 0) {

        int fd = open(source[0], O_RDONLY);
        dup2(fd, 0);
        close(fd);

        execvp(data[0], data);
        perror("redirect in - exec");

        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("redirect in - fork");
    } else {

        do {

            wpid = waitpid(pid, &stat, WUNTRACED);
        } while (!WIFEXITED(stat) && !WIFSIGNALED(stat));
    }

    return 1;
}



int mshell_execute(char **data, int *status) {

    int i;
    int words_amount = status[2];

    if (data == NULL)
        return 1;


    for (i = 0; i < words_amount - 1; i++) {
        if (strcmp(data[i], "&") == 0) {
            printf("mshell - command composition\n");
            return 1;
        }
    }

    if (strcmp(data[words_amount - 1], "&") == 0) {

        status[3] += 1;
        data[words_amount - 1] = NULL;
        return mshell_background(data, status);
    } else {
        data = (char **) realloc(data, words_amount * sizeof(char *));
        data[words_amount] = NULL;
        status[2] += 1;
    }

    if (data[0] == NULL) {
        return 1;
    }

    for (i = 0; i < mshell_builtins_am(); i++) {
        if (strcmp(data[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(data);
        }
    }

    return mshell_forks(data);
}

// background, 2 functions
int mshell_background(char **data, int *status) {
    /* background implementation */

    int pid;

    int fd_hold = dup(0);

    int fd = open("/dev/null", O_RDONLY);
    dup2(fd, 0);
    close(fd);

    signal(SIGINT, SIG_IGN);

    if ((pid = fork()) == 0) {

        execvp(data[0], data);
        perror("background - exec");

        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("background - fork");
    } else {

        int i;

        for (i = 0; i < 256; i++) {
            if (background_pid[i] == 0) {
                background_pid[i] = pid;
                break;
            }
        }

        if (i < 256)
            printf ("[%d] %d \n", (i+1), pid);
        else
            printf("Error - background process table overflow\n");

    }

    dup2(fd_hold, 0);
    return 1;
}
void mshell_background_help() {
    /* checking status of background programs*/
    int i;

    for (i = 0; i < 256; i++) {

        int pid = background_pid[i];

        if (pid != 0) {
            int stat;
            int wpid = waitpid(pid, &stat, WNOHANG);

            if (wpid == 0) {

            } else if (wpid == -1) {
                perror("mshell - background - exit");
            } else {

                if (WIFEXITED(stat))
                    printf("\n[%d] %d - Done\n", (i + 1), pid);
                else
                    printf("\n[%d] %d - Error\n", (i + 1), pid);

                background_pid[i] = 0;
            }
        }
    }

}

// forking new process
int mshell_forks(char **data) {

    int pid, wpid;
    int stat;

    if ((pid = fork()) == 0) {

        signal(SIGINT, SIG_DFL);

        execvp(data[0], data);
        perror("mshell - exec");

        exit(EXIT_FAILURE);

    } else if (pid < 0) {
        perror("mshell - fork");
    } else {

        do {

            wpid = waitpid(pid, &stat, WUNTRACED);
        } while (!WIFEXITED(stat) && !WIFSIGNALED(stat));
    }

    return 1;
}

// builtins - related functions
int mshell_builtins_am() {

    return sizeof(builtin_str) / sizeof(char *);
}
int mshell_cd(char **data) {

    if (data[1] == NULL) {


        char *buffer = (char *) malloc(1024 * sizeof(char));
        memmove(buffer, "/home/ilidannaga", 16);
        data[1] = buffer;

        if (chdir(data[1]) != 0)
            perror("mshell - cd");

    } else {

        if (data[1][0] == '~') {

            char *buffer = (char *) malloc(1024 * sizeof(char));

            memmove(buffer, "/home/ilidannaga", 16);

            int i;

            for (i = 1; i <= strlen(data[1]); i ++) {
                buffer[15 + i] = data[1][i];
            }

            free(data[1]);
            data[1] = buffer;

        }

        if (chdir(data[1]) != 0)
            perror("mshell - cd");
    }


    return 1;
}
int mshell_exit(char **data) {
    return 0;
}
int mshell_help(char **data) {
    /* printing all background data */

    int i;

    printf("Hello, these are all the builtins already realised\n");

    for (i = 0; i < sizeof(builtin_str) / sizeof(char *); i++)
        printf("%s  ", builtin_str[i]);

    printf("\n");

    return 1;
}


//factory
int mshell_factory(char ***data, int commands_amount) {

    int pid, i = 0, status;
    int fd[2];

    while (i < commands_amount) {

        pipe(fd);
        pid = fork();

        if (pid == 0) {

            if (i + 1 != commands_amount) {

                dup2(fd[1], 1);
            }
            close(fd[1]);
            close(fd[0]);
            execvp(data[i][0], data[i]);
            perror("Transporter - exec");
            exit(EXIT_FAILURE);
        }
        dup2(fd[0],0);
        close(fd[1]);
        close(fd[0]);

        i++;
    }

    while (wait(NULL) != -1);

    return 1;

}