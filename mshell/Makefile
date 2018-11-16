all: wtf.o mshell.o reboot_of_shell.o recreating_background.o
	gcc -o shell -Wall wtf.o
	gcc -o shell2 -Wall mshell.o
	gcc -o shell3 -Wall reboot_of_shell.o
	gcc -o shell1 -Wall recreating_background.c
wtf.o: wtf.c
	gcc -c wtf.c

mshell.o: mshell.c
	gcc -c mshell.c

reboot_of_shell.o: reboot_of_shell.c
	gcc -c reboot_of_shell.c

recreating_background.o: recreating_background.c
	gcc -c recreating_background.c

clear:
	rm *.o
