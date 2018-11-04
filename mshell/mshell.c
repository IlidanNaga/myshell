#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>
#include <sys/types.h>

void mshell_init(void);                 // function which makes everything work
char *mshell_getline(int *status);      // getting line until ; or EOF
char **mshell_getwords(char *buffer);   // splitting buffer into the words, filling last part with NULL
int mshell_execute(char **data);        // working with data we got in _getwords, must be splitted into xtra's

int mshell_forks(char **data);          // working with data - fork way
int mshell_background(char **data);     // working with data - background

int mshell_cd(char **data);             // working with data - got cd
int mshell_exit(char **data);           // working with data - got exit
int mshell_builtins_am();               // returning amount of builtin functions we realise


char *builtin_str[] = {
        "cd", "exit"
};

int (*builtin_func[]) (char **) = {
        &mshell_cd, &mshell_exit
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

char** mshell_getwords(char *buffer) {

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

    data = (char**) realloc(data, (words_amount+1) * sizeof(char*));
    data[words_amount] = NULL;

    return data;

}

int mshell_background(char **data) {

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
            perror("mshell - cd");
    }

    return 1;
}
int mshell_exit(char **data) {
    return 0;
}

int mshell_execute(char **data) {

    int i;

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

    int status[2] = {0, 0};

    do {

        printf ("> ");

        buffer = mshell_getline(status);
        data = mshell_getwords(buffer);
        status[1] = mshell_execute(data);

        free(buffer);
        free(data);

    } while (status[0] && status[1]);

}
