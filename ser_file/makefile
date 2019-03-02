all: ser

ser: ser.o work_pthread.o
	gcc -o ser ser.o work_pthread.o -lpthread

ser.o:ser.c
	gcc -c ser.c

work_pthread.o: work_pthread.c
	gcc -c work_pthread.c

clean:
	rm -f *.o ser
