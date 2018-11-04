#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>
#include <sys/types.h>

void mshell_init(void);                 // function which makes everything work
char *mshell_getline(int *status);      // getting line until ; or EOF
char **mshell_getwords(char *buffer, int *status);   // splitting buffer into the words, filling last part with NULL
int mshell_execute(char **data, int *status);        // working with data we got in _getwords, must be splitted into xtra's

int mshell_forks(char **data);                       // working with data - fork way
int mshell_background(char **data);                  // working with data - background, equal to forks but without wait

int mshell_cd(char **data);                          // working with data - got cd
int mshell_exit(char **data);                        // working with data - got exit
int mshell_help(char **data);                        // printing all builtins i made
int mshell_builtins_am();                            // returning amount of builtin functions we realise

int mshell_background2(char **data, int *status);    // hm... pending edition of background

char *builtin_str[] = {
        "cd", "exit","help"
};

int (*builtin_func[]) (char **) = {
        &mshell_cd, &mshell_exit, &mshell_help
};


int main() {
	
	mshell_init();
	
	return 0;
	
}

#define we_add_len 20
char* mshell_getline(int *status) {

    char buff;
    char *buffer = NULL;

    int buffer_len = 0;

    while((scanf("%c", &buff)) != EOF) {

        if (buff != '\n' && buff != ';') {

            if (buffer_len % we_add_len == 0)
                buffer = (char *) realloc(buffer, (buffer_len + we_add_len) * sizeof(char));

            buffer[buffer_len] = buff;
            buffer_len ++;

            status[0] = 0;

        } else {
            status[0] = 1;
            break;

        }
    }

    buffer[buffer_len] = '\0';
    return buffer;
}

char** mshell_getwords(char *buffer, int *status) {

    int i, flag = 0, len = 0;
    char **data = NULL;
    char *buffer2 = NULL;

    int buffer_size = strlen(buffer);
    int words_amount = 0;

    if (buffer_size == 0)
        return NULL;

    for (i = 0; i < buffer_size; i++) {

        if (buffer[i] == ' ') {                      // just ignoring all _
        } else {                                     // if not _

            if (buffer[i] == '&') {

                words_amount++;
                data = (char **) realloc(data, words_amount * sizeof(char *));
                data[words_amount - 1] = (char *) malloc(2 * sizeof(char));
                data[words_amount - 1] = "&";

                i++;

            } else if (buffer[i] == '"') {                  // variant of "WORD"

                i++;

                words_amount++;
                data = (char **) realloc(data, words_amount * sizeof(char*));
                len = 0;

                while (i < buffer_size) {

                    if (buffer[i] != '"') {

                        if (len % we_add_len == 0)
                            buffer2 = (char*) realloc(buffer2, (len + we_add_len)*sizeof(char));

                        buffer2[len] = buffer[i];
                        len++;
                    } else {
                        break;
                    }

                    i++;

                }

                data[words_amount - 1] = buffer2;
                buffer2 = NULL;

                len = 0;
            } else {                                 // variant of ____WORD____

                words_amount++;
                data = (char **) realloc(data, (words_amount) * sizeof(char*));
                len = 0;

                while (i < buffer_size) {

                    if (buffer[i] !=' ' && buffer[i] != '\n' && buffer[i] != '&') {

                        if (len % we_add_len == 0)
                            buffer2 = (char*) realloc(buffer2, (len + we_add_len) * sizeof(char));

                        buffer2[len] = buffer[i];
                        len++;
                    } else {
                        break;
                    }

                    i++;

                }

                data[words_amount - 1] = buffer2;
                buffer2 = NULL;

                len = 0;
            }
        }

    }

    status[2] = words_amount;

    return data;

}

int mshell_background(char **data) {

    int pid, wpid;
    int stat;

    if (pid = fork() == 0) {

        if (execvp(data[0], data) == -1) {
            perror("mshell - exec");
        }

        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("mshell - fork");
    }

    return 1;
}
int mshell_forks(char **data) {

    int pid, wpid;
    int stat;

    if (pid = fork() == 0) {

        if (execvp(data[0], data) == -1) {
            perror("mshell - exec");
        }

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

int mshell_builtins_am() {

    return sizeof(builtin_str) / sizeof(char *);
}
int mshell_cd(char **data) {

    if (data[1] == NULL) {
        perror("mshell: waiting arguments for cd\n");
    } else {
        if (chdir(data[1]) != 0)
            perror("mshell - cd\n");
    }


    return 1;
}
int mshell_exit(char **data) {
    return 0;
}
int mshell_help(char **data) {

    int i;

    printf("Hello, these are all the builtins already realised\n");

    for (i = 0; i < sizeof(builtin_str) / sizeof(char *); i++)
        printf("%s  ", builtin_str[i]);

    printf("\n");

    return 1;
}

int mshell_execute(char **data, int *status) {

    int i;
    int words_amount = status[2];

    for (i = 0; i < words_amount - 1; i++) {
        if (strcmp(data[i], "&") == 0) {
            printf("mshell - command composition\n");
            return 1;
        }
    }

    if (strcmp(data[words_amount - 1], "&") == 0) {

        status[3] += 1;
        data[words_amount - 1] = NULL;
        return mshell_background(data);
    } else {
        data = (char **) realloc(data, words_amount * sizeof(char *));
        data[words_amount] = NULL;
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

void mshell_init(void) {

    char *buffer;
    char **data;

    int status[4] = {0, 0, 0, 0};

    // status[2] is amount of words in command
    // status[3] is amount of running background programs

    do {

        printf ("> ");

        buffer = mshell_getline(status);
        data = mshell_getwords(buffer,status);
        status[1] = mshell_execute(data, status);

        free(buffer);
        free(data);

    } while (status[0] && status[1]);

}


int mshell_background2(char **data, int *status) {

    int pid,pid1, wpid;
    int stat;

    if (pid = fork() == 0) {

         if (pid1 = fork() == 0) {
             if (execvp(data[0], data) == -1) {
                 perror("mshell - exec");
             }

             exit(EXIT_FAILURE);

         } else {

             if (pid1 < 0) {

                 perror("mshell - fork");
             } else {

                 printf("[%d], %d\n", status[3], pid1);

                 do {

                     wpid = waitpid(pid, &stat, WUNTRACED);
                 } while (!WIFEXITED(stat) && !WIFSIGNALED(stat));

                 printf("[%d], Done, %d\n", status[3],wpid);
                 status[3] -= 1;
             }
         }

    } else if (pid < 0) {
        perror("mshell - fork");
    }

    return 1;
}
