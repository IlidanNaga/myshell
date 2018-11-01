#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>


#ifndef myfunc_h
#define myfunc_h

#define we_add_len 10

size_t words_amount = 0;
size_t buffer_size = 0;
size_t pos = 0;

void clear_rubbish(char ***);
char* reading();
char** making_into_words(char *);
int checking_if_all();
int working_with_data(char **);
void make_all_done();

        
#endif

