#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <limits.h>


enum lexemes {NUL, SC, IN, OUT, ADD, PIPE, OR, BACK, AND, LP, RP, END, ERROR, WORD, EndFile, LP_inside, RP_inside, AND_target, OR_target};
#define enum_amount OR_target

struct command {

    char **data;
    char *relocate_in;
    char *relocate_out;
    int data_len;
    int status[OR_target];
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

struct command *mshell_build(int *status);
int and_one_more(struct command *data, int *status);



char *builtin_str[] = {
        "cd", "exit","help"
};
int (*builtin_func[]) (char **) = {
        &mshell_cd, &mshell_exit, &mshell_help
};

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

    struct command *data = NULL;
    int *build = NULL;

    int status[4] = {0, 0, 0, 0};

    // status[0] is error trigger
    // status[2] is amount of commands
    // status[3] is length of our build-like data implementation
    // build - like implementation lays in int *build

    do {

        char path[PATH_MAX];

        getcwd(path, PATH_MAX);

        printf("%s: ", path);

        data = mshell_build(status);

        int i, j;

        printf("commands amount %d\n", status[2]);

        for (i = 0; i < status[2]; i++) {
            printf("Words amount in command %d\n", data[i].data_len);
            if (data[i].status[IN])
                puts(data[i].relocate_in);
            if (data[i].status[OUT])
                puts(data[i].relocate_out);
            for (j = 0; j < data[i].data_len - 1; j++)
                puts(data[i].data[j]);
        }

      //  if (!status[0])
        //    status[1] = one_more_execute(data, status, build);
       // else
        status[1] = 1;
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

        printf("We_are_here %c\n", buff);

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
// building struct command array
struct command *mshell_build(int *status) {

    struct command *data = NULL;
    char *buffer = NULL;
    int commands_amount = 0;
    int stat;

    int k;

    char **command_buffer = NULL;
    int command_len = 0;

    int new_stat;
    int used_new = 0;
    int exit_flag = 0;

    int and_flag = 0;
    int or_flag = 0;

    do {


        if (used_new) {
            stat = new_stat;
            used_new = 0;
        } else {

            buffer = NULL;
            stat = mshell_getlex(&buffer, status);
        }

        switch(stat) {

            case WORD:

                data = (struct command *) realloc(data, (commands_amount + 1) * sizeof(struct command));
                data[commands_amount].relocate_in = NULL;
                data[commands_amount].relocate_out = NULL;

                for (k = 0; k < enum_amount; k++)
                    data[commands_amount].status[k] = 0;

                command_len = 0;
                command_buffer = (char **) malloc(sizeof(char *));
                command_buffer[0] = buffer;
                command_len ++;

                used_new = 0;
                do {
                    buffer = NULL;

                    new_stat = mshell_getlex(&buffer, status);

                    switch(new_stat) {

                        case WORD:

                            command_buffer = (char **) realloc(command_buffer, (command_len + 1) * sizeof(char *));
                            command_buffer[command_len] = buffer;
                            command_len ++;

                            break;

                        case IN:

                            buffer = NULL;

                            new_stat = mshell_getlex(&buffer, status);

                            if (new_stat != WORD) {
                                exit_flag = 2;
                                printf("Error - command composition\n");
                                break;
                            }

                            data[commands_amount].relocate_in = buffer;
                            data[commands_amount].status[IN] = 1;
                            break;

                        case OUT:

                            buffer = NULL;

                            new_stat = mshell_getlex(&buffer, status);

                            if (new_stat != WORD) {
                                exit_flag = 2;
                                printf("Error - command composition\n");
                                break;
                            }

                            data[commands_amount].relocate_out = buffer;
                            data[commands_amount].status[OUT] = 1;
                            break;

                        default:

                            used_new = 1;
                            command_buffer = (char **) realloc(command_buffer, (command_len + 1) * sizeof(char *));
                            command_buffer[command_len] = NULL;
                            command_len ++;
                            break;

                    }

                } while (!used_new && !exit_flag);

                data[commands_amount].data = command_buffer;
                data[commands_amount].data_len = command_len;

                data[commands_amount].status[WORD] = 1;

                if (and_flag) {
                    and_flag = 0;
                    data[commands_amount].status[AND_target] = 1;
                }
                if (or_flag) {
                    or_flag = 0;
                    data[commands_amount].status[OR_target] = 1;
                }

                commands_amount ++;

                break;

            case BACK:

                if (commands_amount == 0) {
                    printf("Error - word_composition");
                    exit_flag = 2;
                    break;
                }

                data[commands_amount - 1].status[BACK] = 1;
                break;

            case EndFile:

                exit_flag = 1;
                break;
            case END:

                exit_flag = 1;
                break;

            case ERROR:

                exit_flag = 2;
                break;


            case IN:

                exit_flag = 2;
                printf("Error - command composition\n");
                break;

            case OUT:

                exit_flag = 2;
                printf("Error - command composition\n");
                break;

            case PIPE:

                if (commands_amount == 0) {
                    exit_flag = 2;
                    printf("Error - command composition\n");
                    break;
                }
                data[commands_amount - 1].status[PIPE] = 1;
                break;

            case AND:

                if (commands_amount == 0) {
                    exit_flag = 2;
                    printf("Error - command composition\n");
                    break;
                }
                data[commands_amount - 1].status[AND] = 1;
                and_flag = 1;
                break;

            case OR:

                if (commands_amount == 0) {
                    exit_flag = 2;
                    printf("Error - command composition\n");
                    break;
                }
                data[commands_amount - 1].status[OR] = 1;
                or_flag = 1;
                break;


            default:
                break;

        }
    } while (!exit_flag);

    if (exit_flag == 2)
        status[0] = 1;

    if (commands_amount == 0)
        status[0] = 1;

    status[2] = commands_amount;
    return data;

}

// hub, relocating into command related functions;
int and_one_more(struct command *data, int *status) {

    //we have a structure with:
    //set of lines char **data
    //lines amount (dunno 4 what, but whynot)
    //int status[OR_target] - tells what was used on it, OR_target - the last enumerate we used
    //status[2] - total amount of commands we have

    int i = 0;

    return 1;

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
                //return mshell_factory(command_storage, command_amount);

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
