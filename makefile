all: maketzx
maketzx: maketzx.c
	gcc -o maketzx -m486 -O3 maketzx.c
