#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <limits.h>

void redirect_in(char *path) {

    int fd = open(path, O_RDONLY);
    dup2(fd, 0);
    close(fd);
}

void redirect_out(char *path) {

    int fd = open(path,O_TRUNC | O_WRONLY | O_CREAT, 0777);
    dup2(fd, 1);
    close(fd);
}

void redirect_append(char *path) {

    int fd = open(path, O_CREAT | O_RDWR, 0777);
    lseek(fd, 0L, SEEK_END);
    dup2(fd, 1);
    close(fd);
}

enum lexemes {NUL, SC, IN, OUT, ADD, PIPE, OR, BACK, AND, LP, RP, END, ERROR, WORD, EndFile, LP_inside, RP_inside, AND_target, OR_target};
#define enum_amount OR_target

struct command {

    char **data;
    char *relocate_in;
    char *relocate_out;
    char *relocate_append;
    int data_len;
    int status[OR_target + 1];
};

int background_pid[256] = {0};
int background_amount = 0;

void mshell_init(void);                              // function which makes everything work
int mshell_getlex(char **buffer, int * status);      // returning lexeme type, in **buffer - string of inner data
int mshell_execute(char **data, int *status);        // working with data we got in _getwords, must be splitted into xtra's

int mshell_forks(struct command data, int *err);                       // working with data - fork way
int mshell_background(struct command data);     // one more idea of implementation
void mshell_background_help();                       // function used 4 printing status of programs

int mshell_cd(struct command data, int *err);                          // working with data - got cd
int mshell_exit(struct command data, int *err);                        // working with data - got exit
int mshell_help(struct command data, int *err);                        // printing all builtins i made
int mshell_builtins_am();                            // returning amount of builtin functions we realise

struct command *mshell_build(int *status);
int and_one_more(struct command *data, int *status);
void mshell_conv(struct command *data, int length, int *err);


int and_execute(struct command *data, int *status);
void background_execute(struct command *data, int *status);


int mshell_back_forks(struct command data, int *err);
void mshell_back_conv(struct command *data, int length, int*err);

char *builtin_str[] = {
        "cd", "exit","help"
};
int (*builtin_func[]) (struct command, int *) = {
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
    int status[5] = {0, 0, 0, 0, 0};


    // status[0] is error trigger
    // status[1] is loop trigger
    // status[2] is amount of commands
    // status[3] is length of our build-like data implementation
    // build - like implementation lays in int *build
    // status[4] is background trigger
    do {

        int k;

        for (k = 0; k < 5; k++)
            status[k] = 0;

        char path[PATH_MAX];

        getcwd(path, PATH_MAX);

        printf("%s: ", path);

        data = mshell_build(status);

        /*
        int i, j;

        printf("commands amount %d\n", status[2]);

        for (i = 0; i < status[2]; i++) {
            printf("Words amount in command %d\n", data[i].data_len);
            if (data[i].status[BACK])
                printf("BEEP %d\n", i);
            for (j = 0; j < data[i].data_len - 1; j++)
                puts(data[i].data[j]);
        }
        */

        if (!status[0]) {
            if (!status[4]) {
                status[1] = and_execute(data, status);
            } else {
                status[1] = 1;
                background_execute(data, status);
            }
        } else
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
                data[commands_amount].relocate_append = NULL;

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

                        case ADD:

                            buffer = NULL;

                            new_stat = mshell_getlex(&buffer, status);

                            if (new_stat != WORD) {
                                exit_flag = 2;
                                printf("Error - command composition\n");
                                break;
                            }

                            data[commands_amount].relocate_append = buffer;
                            data[commands_amount].status[ADD] = 1;
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
                status[4] = 1;
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

            case ADD:

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

    int i;
    for (i = 0; i < commands_amount; i++) {
        if ((data[i].status[OR] || data[i].status[AND]) && data[i].status[BACK]) {
            printf("Error - command composition\n");
            status[0] = 1;
        }
        if (data[i].status[OR] && data[i].status[AND]){
            printf("Error - command composition\n");
            status[0] = 1;
        }

        if (data[i].status[ADD] && data[i].status[OUT]) {
            printf("Error - command composition\n");
            status[0] = 1;
        }

        if (data[i].status[BACK] && i != commands_amount - 1) {
            printf("Error - command composition\n");
            status[0] = 1;
        }

    }
    if (exit_flag == 2)
        status[0] = 1;

    if (and_flag || or_flag)
        status[0] = 1;

    if (commands_amount == 0)
        status[0] = 1;

    status[2] = commands_amount;
    return data;

}

// background, 2 functions
int mshell_background(struct command data) {
    /* background implementation */

    int pid;

    int fd_hold = dup(0);

    int fd = open("/dev/null", O_RDONLY);
    dup2(fd, 0);
    close(fd);

    signal(SIGINT, SIG_IGN);

    if ((pid = fork()) == 0) {

        execvp(data.data[0], data.data);
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
                    printf("[%d]+ %d  Done\n", (i + 1), pid);
                else
                    printf("[%d]- %d  Error\n", (i + 1), pid);

                background_pid[i] = 0;
            }
        }
    }

}

// forking new process
int mshell_forks(struct command data, int *err) {

    int pid;
    int stat;

    if ((pid = fork()) == 0) {

        signal(SIGINT, SIG_DFL);

        if (data.status[IN]) {
            redirect_in(data.relocate_in);
        }
        if (data.status[OUT]) {
            redirect_out(data.relocate_out);
        }

        if (data.status[ADD]) {
            redirect_append(data.relocate_append);
        }

        execvp(data.data[0], data.data);
        perror("mshell - exec");

        exit(EXIT_FAILURE);

    } else if (pid < 0) {
        (*err) = 1;
        perror("mshell - fork");
    } else {

        do {

            waitpid(pid, &stat, WUNTRACED);
        } while (!WIFEXITED(stat) && !WIFSIGNALED(stat));

        (*err) = !WIFEXITED(stat);

    }

    return 1;
}
// builtins - related functions
int mshell_builtins_am() {

    return sizeof(builtin_str) / sizeof(char *);
}
int mshell_cd(struct command data, int *err) {

    if (data.data[1] == NULL) {


        char *buffer = (char *) malloc(1024 * sizeof(char));
        memmove(buffer, "/home/ilidannaga", 16);
        data.data[1] = buffer;

        if (chdir(data.data[1]) != 0) {
            (*err) = 1;
            perror("mshell - cd");
        }
    } else {

        if (data.data[1][0] == '~') {

            char *buffer = (char *) malloc(1024 * sizeof(char));

            memmove(buffer, "/home/ilidannaga", 16);

            int i;

            for (i = 1; i <= strlen(data.data[1]); i++) {
                buffer[15 + i] = data.data[1][i];
            }

            free(data.data[1]);
            data.data[1] = buffer;

        }

        if (chdir(data.data[1]) != 0) {
            (*err) = 1;
            perror("mshell - cd");
       }
    }


    return 1;
}
int mshell_exit(struct command data, int *err) {
    return 0;
}
int mshell_help(struct command data, int *err) {
    /* printing all background data */

    int i;

    printf("Hello, these are all the builtins already realised\n");

    for (i = 0; i < sizeof(builtin_str) / sizeof(char *); i++)
        printf("%s  ", builtin_str[i]);

    printf("\n");

    return 1;
}

void mshell_conv(struct command *data, int length, int*err) {

    int pid, i = 0;
    int fd[2];

    int save_in = dup(0), save_out = dup(1);

    /*
    printf("PIPE - length %d\n", length);

    int j;

    for (j = 0; j < length; j ++)
        puts(data[j].data[0]);
    */
    while (i < length) {

        pipe(fd);
        pid = fork();

        if (pid == 0) {

            if (data[i].status[IN])
                redirect_in(data[i].relocate_in);

            if (data[i].status[OUT])
                redirect_out(data[i].relocate_out);
            if (data[i].status[ADD])
                redirect_append(data[i].relocate_append);


            if (i + 1 != length) {

                dup2(fd[1], 1);
            }
            close(fd[1]);
            close(fd[0]);
            execvp(data[i].data[0], data[i].data);
            (*err) = 1;
            perror("Transporter - exec");
            exit(EXIT_FAILURE);
        }
        dup2(fd[0],0);
        close(fd[1]);
        close(fd[0]);

        i++;
    }

    while (wait(NULL) != -1);

    dup2(save_in, 0);
    dup2(save_out, 1);

}

int and_execute(struct command *data, int *status) {

    //we have a structure with:
    //set of lines char **data
    //lines amount (dunno 4 what, but whynot)
    //int status[OR_target] - tells what was used on it, OR_target - the last enumerate we used
    //status[2] - total amount of commands we have

    int iter = 0, j;
    int we_exit = 1;

    int builtin_flag;
    struct command current;

    int and_flag = 0, and_succes;
    int or_flag = 0, or_succes;
    int execution_failed = 0;

    struct command *transporter = NULL;
    int pipe_len;
    int whynot;


    do {

        and_flag = 0;
        or_flag = 0;
        and_succes = 0;
        or_succes = 0;
        current = data[iter];
        whynot = 1;

        if (current.status[AND_target]) {

            and_succes = !execution_failed;
        }

        if (current.status[OR_target]) {

            or_succes = execution_failed;
        }

        if (current.status[AND])
            and_flag = 1;

        if (current.status[OR])
            or_flag = 1;

        if ((current.status[AND_target] && and_succes) || (current.status[OR_target] && or_succes) || (!current.status[AND_target] && !current.status[OR_target])) {

            if (current.status[PIPE]) {

                transporter = (struct command *) malloc(sizeof(struct command));
                pipe_len = 1;
                whynot = 1;
                transporter[0] = current;

                iter++;
                while ((iter < status[2]) && whynot) {

                    if (data[iter].status[AND]) {
                        and_flag = 1;
                        whynot = 0;
                    }
                    if (data[iter].status[OR]) {
                        or_flag = 1;
                        whynot = 0;
                    }

                    if (data[iter].status[PIPE]) {
                        transporter = (struct command *) realloc(transporter,
                                                                 (pipe_len + 1) * sizeof(struct command));
                        transporter[pipe_len] = data[iter];
                        pipe_len++;

                        iter ++;

                    } else {
                        transporter = (struct command *) realloc(transporter,
                                                                 (pipe_len + 1) * sizeof(struct command));
                        transporter[pipe_len] = data[iter];
                        pipe_len++;
                        whynot = 0;
                        iter ++;
                    }
                }

                mshell_conv(transporter, pipe_len, &execution_failed);

                free(transporter);
            } else {

                builtin_flag = 0;
                for (j = 0; j < mshell_builtins_am(); j++) {
                    if (strcmp(current.data[0], builtin_str[j]) == 0) {
                        we_exit = (*builtin_func[j])(current, &execution_failed);
                        builtin_flag = 1;
                    }
                }

                if (!builtin_flag)
                    we_exit = mshell_forks(current, &execution_failed);

            }
        } else {

            while (data[iter].status[PIPE])
                iter++;
        }

        if (whynot) {
            iter++;
        }

        if (and_flag || or_flag) {
        } else {
            execution_failed = 0;
        }

    } while (iter < status[2] && we_exit);

    return we_exit;

}

void background_execute(struct command *data, int *status) {
    //we have a structure with:
    //set of lines char **data
    //lines amount (dunno 4 what, but whynot)
    //int status[OR_target] - tells what was used on it, OR_target - the last enumerate we used
    //status[2] - total amount of commands we have


    int fd_hold = dup(0);

    int fd = open("/dev/null", O_RDONLY);
    dup2(fd, 0);
    close(fd);

    signal(SIGINT, SIG_IGN);

    int iter = 0, j;
    int we_exit = 1;

    int builtin_flag;
    struct command current;

    int and_flag = 0, and_succes;
    int or_flag = 0, or_succes;
    int execution_failed = 0;

    struct command *transporter = NULL;
    int pipe_len;
    int whynot;


    do {

        and_flag = 0;
        or_flag = 0;
        and_succes = 0;
        or_succes = 0;
        current = data[iter];
        whynot = 1;

        if (current.status[AND_target]) {

            and_succes = !execution_failed;
        }

        if (current.status[OR_target]) {

            or_succes = execution_failed;
        }

        if (current.status[AND])
            and_flag = 1;

        if (current.status[OR])
            or_flag = 1;

        if ((current.status[AND_target] && and_succes) || (current.status[OR_target] && or_succes) || (!current.status[AND_target] && !current.status[OR_target])) {

            if (current.status[PIPE]) {

                transporter = (struct command *) malloc(sizeof(struct command));
                pipe_len = 1;
                whynot = 1;
                transporter[0] = current;

                iter++;
                while ((iter < status[2]) && whynot) {

                    if (data[iter].status[AND]) {
                        and_flag = 1;
                        whynot = 0;
                    }
                    if (data[iter].status[OR]) {
                        or_flag = 1;
                        whynot = 0;
                    }

                    if (data[iter].status[PIPE]) {
                        transporter = (struct command *) realloc(transporter,
                                                                 (pipe_len + 1) * sizeof(struct command));
                        transporter[pipe_len] = data[iter];
                        pipe_len++;

                        iter ++;

                    } else {
                        transporter = (struct command *) realloc(transporter,
                                                                 (pipe_len + 1) * sizeof(struct command));
                        transporter[pipe_len] = data[iter];
                        pipe_len++;
                        whynot = 0;
                        iter ++;
                    }
                }

                mshell_back_conv(transporter, pipe_len, &execution_failed);

                free(transporter);
            } else {

                builtin_flag = 0;
                for (j = 0; j < mshell_builtins_am(); j++) {
                    if (strcmp(current.data[0], builtin_str[j]) == 0) {
                        builtin_flag = 1;
                    }
                }

                if (!builtin_flag)
                    we_exit = mshell_back_forks(current, &execution_failed);

            }
        } else {

            while (data[iter].status[PIPE])
                iter++;
        }

        if (whynot) {
            iter++;
        }

        if (and_flag || or_flag) {
        } else {
            execution_failed = 0;
        }

    } while (iter < status[2] && we_exit);

    dup2(fd_hold, 0);
}

int mshell_back_forks(struct command data, int *err) {

    int pid;

    if ((pid = fork()) == 0) {

        if (data.status[IN]) {
            redirect_in(data.relocate_in);
        }
        if (data.status[OUT]) {
            redirect_out(data.relocate_out);
        }

        if (data.status[ADD]) {
            redirect_append(data.relocate_append);
        }

        execvp(data.data[0], data.data);
        perror("mshell - exec");

        exit(EXIT_FAILURE);

    } else if (pid < 0) {
        (*err) = 1;
        perror("mshell - fork");
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

    return 1;
}
void mshell_back_conv(struct command *data, int length, int*err) {

    int pid, i = 0;
    int fd[2];

    int save_in = dup(0), save_out = dup(1);

    /*
    printf("PIPE - length %d\n", length);

    int j;

    for (j = 0; j < length; j ++)
        puts(data[j].data[0]);
    */
    while (i < length) {

        pipe(fd);
        pid = fork();

        if (pid == 0) {

            if (data[i].status[IN])
                redirect_in(data[i].relocate_in);

            if (data[i].status[OUT])
                redirect_out(data[i].relocate_out);
            if (data[i].status[ADD])
                redirect_append(data[i].relocate_append);


            if (i + 1 != length) {

                dup2(fd[1], 1);
            }
            close(fd[1]);
            close(fd[0]);
            execvp(data[i].data[0], data[i].data);
            (*err) = 1;
            perror("Transporter - exec");
            exit(EXIT_FAILURE);
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
        dup2(fd[0],0);
        close(fd[1]);
        close(fd[0]);

        i++;
    }

    dup2(save_in, 0);
    dup2(save_out, 1);

}