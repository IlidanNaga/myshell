#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#define we_add_len 10

size_t words_amount = 0;
size_t buffer_size = 0;
size_t pos = 0;

char* reading() {
    
    char buff;
    char *buffer = NULL;
    
    while ((scanf("%c", &buff)) != EOF) {

        if (buffer_size % we_add_len == 0)
            buffer = (char*) realloc(buffer, (buffer_size + we_add_len) * sizeof(char));

        buffer[buffer_size] = buff;
        buffer_size++;
    
    }
    
    return buffer;
}

char** making_into_words() {
    
    int i, flag = 0, len = 0;
    char **data = NULL;
    
    char *buffer = reading();
    
    if (buffer_size == 0)
        return NULL;
        
    for (i = pos; i < buffer_size; i++) {

        if (buffer[i] == ';') {
            pos = i+1;
            break;
        }

        if (buffer[i] == ' ' || buffer[i] == '\n') {                      // just ignoring all _   and \n just because i cannot stop spamming em
        } else {                                     // if not _

            if (buffer[i] == '"') {                  // variant of "WORD"

                i++;

                words_amount++;
                data = (char**) realloc(data, (words_amount)*sizeof(char*));
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
            } else {                                 // variant of ____WORD____, ignoring \n because i love spamming em

                words_amount++;
                data = (char**) realloc(data, (words_amount) * sizeof(char*));
                len = 0;

                while (i < buffer_size) {

                    if (buffer[i] !=' ' && buffer[i] != '\n') {

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

    }                                                 // i guess we just exit it... or no?

    return data;
    
}   

int checking_if_all() {
    
    if (pos<buffer_size)
        return 1;
    else
        return 0;
} 

void working_with_data() {
    
    char **data=making_into_words();
    
    data = (char**) realloc(data, (words_amount +1) *sizeof(char*));
    data[words_amount] = NULL;
    
    if ((pid=fork()) == 0) {
        
        res = execvp(data[0],data);
        
    } else {
        
        res = wait(&pid);
    }
    
    if (res == -1)
        printf("Program failed to open\n");
        
    words_amount = 0;
    
}

void clear_rubbish(char ***arr) {
    
    int i;

    for (i = 0; i < words_amount; i++)
        free((*arr)[i]);

    free((*arr));
}

int main () {

    // i guess part upstairs just not useful anymore;

    // words amount will count EXACT amount of words

    // dark zone - everything under this is still in work process
    // current state - we can run 1 process with it's parameters (finaly)

    int pid;
    int res, *son;

    data = (char**) realloc(data, (words_amount + 1) * sizeof(char*));
    data[words_amount] = NULL;

    if ((pid=fork()) == 0) {                        // initialising child process

        res = execvp(data[0],data);

    } else {
        res = wait(&pid);
    }

    if (res == -1)
        printf ("Program failed to open\n");

    // please, don't check with vallgrind

    clear_rubbish(&data);
    return 0;

}
