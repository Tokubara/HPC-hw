main: main.c globalinit.c
	mpicc -o $@ $<
run: main
	mpirun -np 6 --oversubscribe main
