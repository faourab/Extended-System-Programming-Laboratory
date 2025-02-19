#format is target-name: target dependencies 

#{-tab-}actions 

 # All Targets 


all: encoder

encoder: encoder.o 
	gcc -m32 -g -Wall -o encoder encoder.o

encoder.o: encoder.c
	gcc -g -Wall -m32 -c -o encoder.o encoder.c


.PHONY: clean 

 #Clean the build directory 

clean:
	rm -f *.o encoder