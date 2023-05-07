#
# This is the Makefile that can be used to create the "warmup2" executable
# To create "warmup2" executable, do:
#	make warmup2
#

warmup2: warmup2.o my402list.o myfunction.o
	gcc -o warmup2 -g warmup2.o my402list.o myfunction.o -lm -pthread

warmup2.o: warmup2.c my402list.h myfunction.h
	gcc -g -c -Wall -lm -pthread warmup2.c

myfunction.o: myfunction.c my402list.h myfunction.h
	gcc -g -c -Wall -lm -pthread myfunction.c

my402list.o: my402list.c my402list.h
	gcc -g -c -Wall my402list.c

clean:
	rm -f *.o warmup2
