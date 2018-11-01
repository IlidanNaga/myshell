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

void clear_rubbish(char ***arr) {
    
    int i;

    for (i = 0; i < words_amount; i++)
        free((*arr)[i]);

    free((*arr));
}

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

char** making_into_words(char *buffer) {
    
    int i, flag = 0, len = 0;
    char **data = NULL;
    char *buffer2 = NULL;
    
    if (buffer_size == 0)
        return NULL;
        
    for (i = pos; i < buffer_size; i++) {

        if (buffer[i] == ';') {
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
    
    pos = i + 1;
    }                                                 // i guess we just exit it... or no?

    data = (char**) realloc(data, (words_amount+1) * sizeof(char*));
    data[words_amount] = NULL;

    return data;
    
}   

int checking_if_all() {
    
    if (pos<buffer_size)
        return 1;
    else
        return 0;
} 

int working_with_data(char **data) {  // array will be preparated - added last NULL line

    int pid;
    int res, *son;
    
    if ((pid=fork()) == 0) {
        
        res = execvp(data[0],data);
        
    } else {
        
        res = wait(&pid);
    }
    
    if (res == -1)
        printf("Program failed to open\n");

    return res;
}

void make_all_done() {
    
    char **data = NULL;
    char *buffer = reading();
    
    int res;
    
    while (checking_if_all()) {
        
        data = making_into_words(buffer);
        
        res = working_with_data(data);
        
        clear_rubbish(&data);
        data = NULL;
        
    }
        
    free(buffer);
    
}

int main () {
    
    make_all_done();
    
    return 0;
}
