all: myshell looper mypipeline

myshell: 
	gcc -c -g -m32 -o myshell.o myshell.c
	gcc -c -g -m32 -o LineParser.o LineParser.c
	gcc -g -m32 -o myshell myshell.o LineParser.o

looper: looper.c
	gcc -g -m32 -o looper looper.c

mypipeline: mypipeline.c
	gcc -g -m32 -o mypipeline mypipeline.c

clean:
	rm -f *.o myshell looper mypipeline