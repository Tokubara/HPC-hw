CFLAGS ?= -std=c++14 -O0 -Wall -Wextra -Werror -pedantic -D NDEBUG

.PHONY: clean check_mpi

all: check_mpi generate odd_even_sort

data_n:=100000000

check_mpi:
ifeq ($(shell which mpicxx),)
	$(error No mpicxx found, please load OpenMPI first!)
endif

generate: generate.cpp
	g++ $(CFLAGS) $^ -o $@ 

odd_even_sort: main.cpp worker.cpp odd_even_sort.cpp
	mpicxx -g $(CFLAGS) $^ -o $@

clean:
	rm -f generate odd_even_sort

input.txt: ./generate
	./generate $(data_n) input.txt

log: odd_even_sort input.txt
	mpirun -n 5 --oversubscribe ./odd_even_sort $(data_n) input.txt &> run_error_log.txt
	# objdump -d -S -l odd_even_sort > objdump_log.txt

