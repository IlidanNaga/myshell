all: wtf.o mshell.o reboot_of_shell.o recreating_background.o brackets_attempt.o
	gcc -o shell4 -Wall wtf.o
	gcc -o shell2 -Wall mshell.o
	gcc -o shell3 -Wall reboot_of_shell.o
	gcc -o shell1 -Wall recreating_background.o
	gcc -o shell -Wall brackets_attempt.o
wtf.o: wtf.c
	gcc -c wtf.c

mshell.o: mshell.c
	gcc -c mshell.c

reboot_of_shell.o: reboot_of_shell.c
	gcc -c reboot_of_shell.c

recreating_background.o: recreating_background.c
	gcc -c recreating_background.c

brackets_attempt.o: brackets_attempt.c
	gcc -c brackets_attempt.c
clear:
	rm *.o
