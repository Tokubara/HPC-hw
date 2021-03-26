#include <algorithm>
#include <iostream>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
using namespace std;
int compare(const void* a, const void* b)
{
    return (*(int*)a > *(int*)b);
}

int main(int argc, char* argv[])
{

    int nump, rank;
    int n, localn;
    int *data, recdata[100], recdata2[100];
    int* temp;
		int* send_count, *displ;
    int ierr, i;
    int root_process;
    int sendcounts;
    MPI_Status status;

    ierr = MPI_Init(&argc, &argv);
    root_process = 0;
    ierr = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    ierr = MPI_Comm_size(MPI_COMM_WORLD, &nump);

    if (rank == root_process) {
	printf("please enter the number of numbers to sort: ");
	fflush(stdout);
	scanf("%i", &n);

	data = (int*)malloc(sizeof(int) * n);
	for (i = 0; i < n; i++) {   // 手动制造数据
	    data[i] = rand() % 100; // 为什么%100? 为什么限制在100以内. 因为每个进程的数据的规模在100以内.
	}
	send_count = (int*) malloc(sizeof(int)*nump);
    int avgn = n / nump;
for (i = 0; i < nump; i++) { 
	send_count[i] = avgn;
}	
send_count[nump-1]= (n - (nump - 1) * avgn);
displ[0]=0;
for(int i = 1; i < nump; i++) {
	displ[i]=displ[i-1]+send_count[i-1];
}
    } else {
	data = NULL;
    }
    int avgn = n / nump;
    localn = (rank == nump - 1) ? (n - (nump - 1) * avgn) : avgn;
    /* ierr = MPI_Bcast(&localn, 1, MPI_INT, 0, MPI_COMM_WORLD); */
    ierr = MPI_Scatterv(data, send_count, displ, MPI_INT, &recdata, localn, MPI_INT, 0, MPI_COMM_WORLD); // 接受是100, 这说明, 100其实只是起了这么大buffer的作用, 只要有足够的buffer, 想多大就多大
		//? 其它的没有开辟空间, 会不会出事啊?
    sort(recdata, recdata + localn);

    // {{{1 设置oddrank,evenrank, 也就是partner序号
    //begin the odd-even sort
    int oddrank, evenrank; // 变量的含义是奇/偶时的partner,

    if (rank % 2 == 0) {
	oddrank = rank - 1; // 但是可能是-1
	evenrank = rank + 1;
    } else {
	oddrank = rank + 1;
	evenrank = rank - 1;
    }
    /* Set the ranks of the processors at the end of the linear */
    if (oddrank == -1 || oddrank == nump)
	oddrank = MPI_PROC_NULL; // 可以么?
    if (evenrank == -1 || evenrank == nump)
	evenrank = MPI_PROC_NULL;

    // {{{1 获取partner的长度
    int odd_len, even_len; // 分别是odd/even partner的长度
    MPI_Sendrecv(&localn, 1, MPI_INT, oddrank, 2, &odd_len, 1, MPI_INT, oddrank, 2, MPI_COMM_WORLD, &status);
    MPI_Sendrecv(&localn, 1, MPI_INT, evenrank, 2, &even_len, 1, MPI_INT, evenrank, 2, MPI_COMM_WORLD, &status);

    int p;
    for (p = 0; p < nump - 1; p++) {												 // 为什么只需要迭代固定次数?
	if (p % 2 == 1)														 /* Odd phase */
	    MPI_Sendrecv(recdata, localn, MPI_INT, oddrank, 1, recdata2, odd_len, MPI_INT, oddrank, 1, MPI_COMM_WORLD, &status); // 所有进程都可以只用这个编号
	else															 /* Even phase */
	    MPI_Sendrecv(recdata, localn, MPI_INT, evenrank, 1, recdata2, even_len, MPI_INT, evenrank, 1, MPI_COMM_WORLD, &status);

	//extract localn after sorting the two
	temp = (int*)malloc(localn * sizeof(int));
	for (i = 0; i < localn; i++) {
	    temp[i] = recdata[i]; // 拷贝到temp[i]
	}
	if (status.MPI_SOURCE == MPI_PROC_NULL)
	    continue;
	else if (rank < status.MPI_SOURCE) { // 这真的可以做到么?
	    //store the smaller of the two
	    int i, j, k;
	    for (i = j = k = 0; k < localn; k++) {
		if (j == localn || (i < localn && temp[i] < recdata2[j]))
		    recdata[k] = temp[i++];
		else
		    recdata[k] = recdata2[j++];
	    }
	} else {
	    //store the larger of the two
	    int i, j, k;
	    for (i = j = k = localn - 1; k >= 0; k--) {
		if (j == -1 || (i >= 0 && temp[i] >= recdata2[j]))
		    recdata[k] = temp[i--];
		else
		    recdata[k] = recdata2[j--];
	    }
	} //else
    }	  //for

    ierr = MPI_Gatherv(recdata, localn, MPI_INT, data, send_count, displ, MPI_INT, 0, MPI_COMM_WORLD);
    int error = 0;
    if (rank == root_process) {
	for (int i = 0; i < n - 1; i++) {
	    if (data[i] > data[i + 1]) {
		error = 1;
	    }
	}
	if (error == 1) {
	    puts("error");
	} else {
	    puts("ok");
	}
    }

    ierr = MPI_Finalize();
}
