#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <limits.h>

int allowed_to_print = 0;
char ideal[PATH_MAX];
int length_ideal = 0;
int in;
int in_file = 0;

int when_entered;
int restore;

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
    char *brackets;
    int data_len;
    int status[OR_target + 1];
};

int background_pid[256] = {0};
int background_amount = 0;

void mshell_init(void);                              // function which makes everything work
int mshell_getlex(char **buffer, int * status);      // returning lexeme type, in **buffer - string of inner data
int mshell_getlex_file(char **buffer, int *status);  // returning lexeme type, in **buffer - string of inner data;

int (*getlex_types[]) (char **buffer, int *status) = {
    &mshell_getlex, &mshell_getlex_file
};

int mshell_execute(char **data, int *status);        // working with data we got in _getwords, must be splitted into xtra's

int mshell_forks(struct command data, int *err);                       // working with data - fork way
void mshell_background_help();                       // function used 4 printing status of programs

int mshell_cd(struct command data, int *err);                          // working with data - got cd
int mshell_exit(struct command data, int *err);                        // working with data - got exit
int mshell_help(struct command data, int *err);                        // printing all builtins i made
int mshell_builtins_am();                            // returning amount of builtin functions we realise

struct command *mshell_build(int *status);
void mshell_conv(struct command *data, int length, int *err);


int and_execute(struct command *data, int *status);
void background_execute(struct command *data, int *status);


int mshell_back_forks(struct command data, int *err);
void mshell_back_conv(struct command *data, int length, int*err);

void free_commands(struct command *data, int commands_amount);

char *builtin_str[] = {
        "cd", "exit","help"
};
int (*builtin_func[]) (struct command, int *) = {
        &mshell_cd, &mshell_exit, &mshell_help
};

// "main" - literaly does nothing, just redirects input if we get file in
int main(int argc, char **argv) {

    signal(SIGINT, SIG_IGN);

    if (argc >= 2) {
        in = open(argv[1], O_RDONLY);
        in_file = 1;
        allowed_to_print = 0;
    } else {
        allowed_to_print = 1;
    }

    /*

    save_fd = open("system_useage_pidsave.txt", O_CREAT | O_RDONLY, 0777);
    int successful_read = read(save_fd, &read_fd, sizeof(int));

    if (successful_read > 0) {
        if (getppid() == read_fd)
            allowed_to_print = 0;
    } else {
        allowed_to_print = 1;
    }

    close(save_fd);

    save_fd = open("system_useage_pidsave.txt", O_CREAT | O_WRONLY | O_TRUNC, 0777);
    int writing = getpid();
    write(save_fd, &writing, sizeof(int));
    close(save_fd);

    */

    int pid = fork(), stat;

    if (pid) {

        do {

            waitpid(pid, &stat, WUNTRACED);
        } while (!WIFEXITED(stat) && !WIFSIGNALED(stat));

        char arr[PATH_MAX];
        int fd = open("asdf.txt", O_RDONLY);

        arr[0] = '/';
        arr[1] = 'h';
        arr[2] = 'o';
        arr[3] = 'm';
        arr[4] = 'e';
        arr[5] = '/';
        read(fd, arr+6, PATH_MAX);
        close(fd);

        arr[strlen(arr) - 1] = ' ';

        int i;

        for (i = 0; i < strlen(arr) - 1; i++)
            ideal[i] = arr[i];

        length_ideal = strlen(arr) - 1;


    } else {

        int fd = open("asdf.txt", O_CREAT | O_WRONLY | O_TRUNC, 0777);
        dup2(fd, 1);
        close(fd);
        execlp("whoami", "whoami", NULL);
        perror("exec");
    }

    mshell_init();

    /*

    save_fd = open("system_useage_pidsave.txt", O_TRUNC);
    close(save_fd);

    */

    return 0;

}

#define we_add_len 20                   // const used to increase buffer size
void mshell_init(void) {
    /* hub, controls the work of the program*/

    struct command *data = NULL;
    int status[7] = {0, 0, 0, 0, 0, 0, 0};
    int flag;

    // status[0] is error trigger
    // status[1] is loop trigger
    // status[2] is amount of commands
    // status[3] is length of our build-like data implementation
    // build - like implementation lays in int *build
    // status[4] is background trigger
    // status[5] is brackets trigger (dunno if we have 2 use)
    // status[6] is EOF trigger - used to exit the fucking shell;

    do {

        int k;

        for (k = 0; k < 6; k++)
            status[k] = 0;

        if (allowed_to_print) {

            char path[PATH_MAX];

            getcwd(path, PATH_MAX);

            flag = 1;

            for (k = 0; k < length_ideal; k++) {
                if (path[k] != ideal[k])
                    flag = 0;
            }

            if (!flag) {
                printf("%s$ ", path);
            } else {
                char path1[PATH_MAX];
                path1[0] = '~';

                for (k = 1; k < PATH_MAX - length_ideal; k++)
                    path1[k] = path[length_ideal - 1 + k];

                printf("%s$ ", path1);
            }

        }

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
    } while (status[1]  && !status[6]);

}
// getting lexemes

int mshell_getlex_file(char **buffer, int *status) {

    char buff;
    char *local_buffer = NULL;
    int local_len = 0;
    int current_len = 0;
    (*buffer) = NULL;
    int flag = 0;
    int read_no;

    while(1) {


        read_no = read(in, &buff, 1);

        if (read_no < 1) {
            status[6] = 1;
            return END;
        }

        switch (buff) {

            case '\033':

                printf("BEEP\n");

                break;

            case '|':

                read_no = read(in, &buff, 1);

                if (read_no < 1) {
                    status[6] = 1;
                    return ERROR;
                }

                if (buff == '|')
                    return OR;

                lseek(in, -1, SEEK_CUR);
                return PIPE;
            case ';':

                return END;
            case '<':

                return IN;
            case '>':

                read(in, &buff, 1);

                if (buff == '>')
                    return ADD;
                else {
                    lseek(in, -1, SEEK_CUR);
                    return OUT;
                }
            case '&':

                read(in, &buff, 1);
                if (buff == '&')
                    return AND;
                else {
                    lseek(in, -1, SEEK_CUR);
                    return BACK;
                }
                break;
            case '(':

                current_len = 0;

                read_no = read(in, &buff, 1);

                if (read_no < 1) {
                    status[6] = 1;
                    return ERROR;
                }

                flag ++;

                while (flag) {

                    if (buff == '\n')
                        return ERROR;


                    if (current_len % we_add_len == 0)
                        local_buffer = (char *) realloc(local_buffer, (current_len + we_add_len) * sizeof(char));

                    local_buffer[current_len] = buff;
                    current_len ++;

                    read(in, &buff, 1);

                    if (buff == '(')
                        flag ++;
                    if (buff == ')')
                        flag --;

                }

                if (current_len % we_add_len == 0)
                    local_buffer = (char *) realloc(local_buffer, (current_len + 1) * sizeof(char));

                local_buffer[current_len] = EOF;


                (*buffer) = local_buffer;
                return LP;

                break;
            case ')':
                return RP;   // later implement a dead-end if we meet a RP
                break;
            case EOF:
                status[6] = 1;
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

                read(in, &buff, 1);

                while (buff != '"') {

                    if (buff == '\n' || buff == EOF)
                        return ERROR;

                    if (local_len % we_add_len == 0)
                        local_buffer = (char *) realloc(local_buffer, (local_len + we_add_len) * sizeof(char));

                    local_buffer[local_len] = buff;
                    local_len++;

                    read(in, &buff, 1);
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

                    read(in, &buff, 1);
                }

                (*buffer) = local_buffer;
                lseek(in, -1, SEEK_CUR);
                return WORD;
                break;
        }
    }
}

int mshell_getlex(char **buffer, int *status) {
    /* getting lexemes \ words */

    char buff;
    char *local_buffer = NULL;
    int local_len = 0;
    int current_len = 0;
    (*buffer) = NULL;

    int flag = 0;

    while(1) {

        buff = getchar();


        switch (buff) {

            case '\033':

                printf("BEEP\n");

                break;

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

                current_len = 0;
                buff = getchar();
                flag ++;

                while (flag) {

                    if (buff == '\n')
                        return ERROR;

                    if (buff == EOF) {
                        status[6] = 1;
                        return ERROR;
                    }

                    if (current_len % we_add_len == 0)
                        local_buffer = (char *) realloc(local_buffer, (current_len + we_add_len) * sizeof(char));

                    local_buffer[current_len] = buff;
                    current_len ++;

                    buff = getchar();

                    if (buff == '(')
                        flag ++;
                    if (buff == ')')
                        flag --;

                }

                if (current_len % we_add_len == 0)
                    local_buffer = (char *) realloc(local_buffer, (current_len + 1) * sizeof(char));

                local_buffer[current_len] = EOF;


                (*buffer) = local_buffer;
                return LP;

                break;
            case ')':
                return RP;   // later implement a dead-end if we meet a RP
                break;
            case EOF:
                status[6] = 1;
                return END;
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

    char *buffer_in = NULL;
    char *buffer_out = NULL;
    char *buffer_append = NULL;

    int in_flag = 0;
    int out_flag = 0;
    int append_flag = 0;

    do {


        if (used_new) {
            stat = new_stat;
            used_new = 0;
        } else {

            buffer = NULL;
            stat = getlex_types[in_file](&buffer, status);
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

                if (in_flag) {
                    in_flag = 0;
                    data[commands_amount].relocate_in = buffer_in;
                    data[commands_amount].status[IN] = 1;
                }

                if (out_flag) {
                    out_flag = 0;
                    data[commands_amount].relocate_out = buffer_out;
                    data[commands_amount].status[OUT] = 1;
                }

                if (append_flag) {
                    append_flag = 0;
                    data[commands_amount].relocate_append = buffer_append;
                    data[commands_amount].status[ADD] = 1;
                }

                do {
                    buffer = NULL;

                    new_stat = getlex_types[in_file](&buffer, status);

                    switch(new_stat) {

                        case WORD:

                            command_buffer = (char **) realloc(command_buffer, (command_len + 1) * sizeof(char *));
                            command_buffer[command_len] = buffer;
                            command_len ++;

                            break;

                        case IN:

                            buffer = NULL;

                            new_stat = getlex_types[in_file](&buffer, status);

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

                            new_stat = getlex_types[in_file](&buffer, status);

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

                            new_stat = getlex_types[in_file](&buffer, status);

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

                if (status[6])
                    printf("\nError - command composition");
                else
                    printf("Error - command composition\n");
                exit_flag = 2;
                break;


            case IN:

                if (!in_flag) {
                    in_flag = 1;
                    buffer = NULL;

                    new_stat = getlex_types[in_file](&buffer, status);

                    if (new_stat != WORD) {
                        exit_flag = 2;
                        printf("Error - command composition\n");
                        break;
                    }

                    buffer_in = buffer;
                    break;

                }

                exit_flag = 2;
                printf("Error - command composition\n");
                break;

            case OUT:

                if (!out_flag) {
                    out_flag = 1;
                    buffer = NULL;

                    new_stat = getlex_types[in_file](&buffer, status);

                    if (new_stat != WORD) {
                        exit_flag = 2;
                        printf("Error - command composition\n");
                        break;
                    }

                    buffer_out = buffer;
                    break;

                }

                exit_flag = 2;
                printf("Error - command composition\n");
                break;

            case ADD:


                if (!append_flag) {
                    append_flag = 1;
                    buffer = NULL;

                    new_stat = getlex_types[in_file](&buffer, status);

                    if (new_stat != WORD) {
                        exit_flag = 2;
                        printf("Error - command composition\n");
                        break;
                    }

                    buffer_append = buffer;
                    break;

                }

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

            case RP:

                exit_flag = 2;
                printf("Error - command composition\n");
                break;

            case LP:

                data = (struct command *) realloc(data, (commands_amount + 1) * sizeof(struct command));
                data[commands_amount].relocate_in = NULL;
                data[commands_amount].relocate_out = NULL;
                data[commands_amount].relocate_append = NULL;
                data[commands_amount].data = NULL;
                data[commands_amount].brackets = buffer;
                data[commands_amount].data_len = 0;

                for (k = 0; k < enum_amount; k++)
                    data[commands_amount].status[k] = 0;

                data[commands_amount].status[LP] = 1;

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

    int flag_safe;
    char it_s_hold[2] = {'1', '1'};

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

        flag_safe = open("status_and_or.txt", O_CREAT | O_TRUNC | O_WRONLY, 0777);
        write(flag_safe, it_s_hold, 1);
        close(flag_safe);

        exit(EXIT_FAILURE);

    } else if (pid < 0) {
        flag_safe = open("status_and_or.txt", O_CREAT | O_TRUNC | O_WRONLY, 0777);
        write(flag_safe, it_s_hold, 1);
        close(flag_safe);
        perror("mshell - fork");
    } else {

        do {

                waitpid(pid, &stat, WUNTRACED);
            } while (!WIFEXITED(stat) && !WIFSIGNALED(stat));

    }

    return 1;
}
// builtins - related functions
int mshell_builtins_am() {

    return sizeof(builtin_str) / sizeof(char *);
}
int mshell_cd(struct command data, int *err) {

    int flag_safe;
    char it_s_hold[2] = {'1', '1'};

    if (data.data[1] == NULL) {

        data.data[1] = ideal;

        if (chdir(data.data[1]) != 0) {
            flag_safe = open("status_and_or.txt", O_CREAT | O_TRUNC | O_WRONLY, 0777);
            write(flag_safe, it_s_hold, 1);
            close(flag_safe);
            perror("mshell - cd");
        }
    } else {

        if (data.data[1][0] == '~') {

            char buffer[PATH_MAX];
            int i;

            for (i = 0; i < length_ideal; i++) {
                buffer[i] = ideal[i];
            }

            for (i = 1; i <= strlen(data.data[1]); i++) {
                buffer[length_ideal - 1 + i] = data.data[1][i];
            }

            free(data.data[1]);
            data.data[1] = buffer;

        }

        if (chdir(data.data[1]) != 0) {
            flag_safe = open("status_and_or.txt", O_CREAT | O_TRUNC | O_WRONLY, 0777);
            write(flag_safe, it_s_hold, 1);
            close(flag_safe);
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

//transporter
void mshell_conv(struct command *data, int length, int*err) {

    int pid, i = 0;
    int fd[2];

    int save_in = dup(0), save_out = dup(1);

    int flag_safe;
    char it_s_hold[2] = {'1', '1'};

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

            flag_safe = open("status_and_or.txt", O_CREAT | O_TRUNC | O_WRONLY, 0777);
            write(flag_safe, it_s_hold, 1);
            close(flag_safe);

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

//hub for not background-running function
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
    int pid, stat;
    char it_s_for_flag[2];
    int flag_safe;
    int successful_read;

    flag_safe = open("status_and_or.txt", O_CREAT | O_TRUNC, 0777);
    close(flag_safe);

    dup2(when_entered, 0);

    do {

        and_succes = 0;
        or_succes = 0;
        current = data[iter];
        whynot = 1;

        flag_safe = open("status_and_or.txt", O_RDONLY, 0777);
        successful_read = read(flag_safe, (it_s_for_flag + 1), 1);

        if (successful_read) {
            execution_failed = 1;
        } else {
            execution_failed = 0;
        }
        close(flag_safe);

        flag_safe = open("status_and_or.txt", O_TRUNC);
        close(flag_safe);

        if (current.status[AND_target]) {

            and_succes = !execution_failed;
        }

        if (current.status[OR_target]) {

            or_succes = execution_failed;
        }


        if ((current.status[AND_target] && and_succes) || (current.status[OR_target] && or_succes) ||
            (!current.status[AND_target] && !current.status[OR_target])) {

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

                        iter++;

                    } else {
                        transporter = (struct command *) realloc(transporter,
                                                                 (pipe_len + 1) * sizeof(struct command));
                        transporter[pipe_len] = data[iter];
                        pipe_len++;
                        whynot = 0;
                        iter++;
                    }
                }

                mshell_conv(transporter, pipe_len, &execution_failed);

                free(transporter);
            } else if (current.status[LP]) {

                int fd = open("system_useage_file.txt", O_CREAT | O_WRONLY | O_TRUNC, 0777);
                write(fd, current.brackets, strlen(current.brackets));
                close(fd);

                pid = fork();

                if (pid == 0) {
                    execlp("./shell", "./shell", "system_useage_file.txt", NULL);
                    perror("brackets - exec");

                    exit(EXIT_FAILURE);
                } else if (pid < 0) {
                    perror("brackets - fork");
                } else {

                    do {

                        waitpid(pid, &stat, WUNTRACED);
                    } while (!WIFEXITED(stat) && !WIFSIGNALED(stat));
                }
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

            iter ++;
        }

        iter ++;

    } while (iter < status[2] && we_exit);

    return we_exit;

}

//background implementation
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

    int flag_safe;
    int successful_read;
    char it_s_for_flag[2];

    int pid, stat;


    do {

        and_flag = 0;
        or_flag = 0;
        and_succes = 0;
        or_succes = 0;
        current = data[iter];
        whynot = 1;

        flag_safe = open("status_and_or.txt", O_RDONLY, 0777);
        successful_read = read(flag_safe, (it_s_for_flag + 1), 1);

        if (successful_read) {
            execution_failed = 1;
        } else {
            execution_failed = 0;
        }
        close(flag_safe);

        flag_safe = open("status_and_or.txt", O_TRUNC);
        close(flag_safe);

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

        if (current.status[LP]) {
            if ((current.status[AND_target] && and_succes) || (current.status[OR_target] && or_succes) ||
                (!current.status[AND_target] && !current.status[OR_target])) {

                int fd = open("system_useage_file.txt", O_CREAT | O_WRONLY | O_TRUNC, 0777);
                write(fd, current.brackets, strlen(current.brackets));
                close(fd);
                iter++;

                pid = fork();

                if (pid == 0) {
                    execlp("./shell", "./shell", "system_useage_file.txt", NULL);
                    perror("brackets - exec");

                    exit(EXIT_FAILURE);
                } else if (pid < 0) {
                    perror("brackets - fork");
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
            } else {
                iter++;
            }
        } else {
            if ((current.status[AND_target] && and_succes) || (current.status[OR_target] && or_succes) ||
                (!current.status[AND_target] && !current.status[OR_target])) {

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

                            iter++;

                        } else {
                            transporter = (struct command *) realloc(transporter,
                                                                     (pipe_len + 1) * sizeof(struct command));
                            transporter[pipe_len] = data[iter];
                            pipe_len++;
                            whynot = 0;
                            iter++;
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

        }

    } while (iter < status[2] && we_exit);

    dup2(fd_hold, 0);
}
int mshell_back_forks(struct command data, int *err) {

    int pid;
    int flag_safe;
    char it_s_hold[2] = {'1', '1'};

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
        flag_safe = open("status_and_or.txt", O_CREAT | O_TRUNC | O_WRONLY, 0777);
        write(flag_safe, it_s_hold, 1);
        close(flag_safe);
        perror("mshell - exec");

        exit(EXIT_FAILURE);

    } else if (pid < 0) {
        flag_safe = open("status_and_or.txt", O_CREAT | O_TRUNC | O_WRONLY, 0777);
        write(flag_safe, it_s_hold, 1);
        close(flag_safe);
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

    int flag_safe;
    char it_s_hold[2] = {'1', '1'};

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

            flag_safe = open("status_and_or.txt", O_CREAT | O_TRUNC | O_WRONLY, 0777);
            write(flag_safe, it_s_hold, 1);
            close(flag_safe);

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

void free_commands(struct command *data, int commands_amount) {

    struct command current;
    int i, j;

    for (i = 0; i < commands_amount; i++) {
        current = data[i];

        free(current.relocate_append);
        free(current.relocate_out);
        free(current.relocate_in);

        for (j = 0; j < current.data_len; j ++ ) {
            free(current.data[j]);
        }
        if (current.data_len != 0)
            free(current.data);
    }
}

