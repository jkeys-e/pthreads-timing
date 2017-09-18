all: pthreads-time

pthreads-time: clean
	gcc -pthread pthreads-timing.c -o pthreads-timing
	
clean:
	rm pthreads-timing -f