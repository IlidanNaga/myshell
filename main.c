#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#define we_add_len 10

size_t words_amount = 0;



void clear_rubbish(char ***arr) {
    int i;

    for (i = 0; i < words_amount; i++)
        free((*arr)[i]);

    free((*arr));
}

int main () {

	char buff;
	char *buffer = NULL;
	int buffer_size = 0;

	while ((scanf("%c", &buff)) != EOF) {

		if (buffer_size % we_add_len == 0)
			buffer = (char*) realloc(buffer, (buffer_size + we_add_len) * sizeof(char));

		buffer[buffer_size] = buff;
		buffer_size++;

	}

	char **data = NULL;

	int i = 0, flag = 0, len = 0;
	char *buffer2 = NULL;

    /*	while (flag == 0) {

		if (buffer[i] != ' ') {

			i++;
			pos++

			flag = 1;

			if (buffer[i] == '"')
				space_flag = 1;

			for (k = i; k < buffer_size; k++) {

				if (command_size % we_add_len == 0)
					command = (char*) realloc(command, (command_size + we_add_len) * sizeof(char));

				if (space_flag) {

					if (buffer[k] != '"') {

						command[command_size] = buffer[k];
						command_size++;
					} else {
						break;
					}
				} else {

					if (buffer[k] != ' ') {

						command[command_size] = buffer[k];
						command size++;
					} else {
						break;
					}
				}
			}
			break;

		}

		pos++;
		i++;

	} */

	// i guess part upstairs just not useful anymore;

	// words amount will count EXACT amount of words

	if (buffer_size == 0)
	    return 0;

    for (i = 0; i < buffer_size; i++) {

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

	}                                                // i guess we just exit it... or no?


/*	printf ("\n");
	for (i = 0; i < words_amount; i++) {             // if everything is ok it'll print ALL the words
		puts (data[i]);
	} */

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
