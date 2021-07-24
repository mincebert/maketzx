all: maketzx

remake: clean all

.PHONY: clean
clean:
	-rm maketzx

maketzx: maketzx.c
	gcc -o maketzx -O3 maketzx.c
