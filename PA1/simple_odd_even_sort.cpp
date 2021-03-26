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
	int avgn = n / nump;
	localn = avgn;

	data = (int*)malloc(sizeof(int) * n);
	for (i = 0; i < n; i++) {   // 手动制造数据
	    data[i] = rand() % 100; // 为什么%100? 为什么限制在100以内. 因为数据的规模在100以内.
	}
	printf("array data is:");
	for (i = 0; i < n; i++) {
	    printf("%d ", data[i]);
	}
	printf("\n");
    } else {
	data = NULL;
    }
    ierr = MPI_Bcast(&localn, 1, MPI_INT, 0, MPI_COMM_WORLD);
    ierr = MPI_Scatter(data, localn, MPI_INT, &recdata, 100, MPI_INT, 0, MPI_COMM_WORLD);
    printf("%d:received data:", rank);
    for (i = 0; i < localn; i++) {
	printf("%d ", recdata[i]);
    }
    printf("\n");
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

    ierr = MPI_Gather(recdata, localn, MPI_INT, data, localn, MPI_INT, 0, MPI_COMM_WORLD);
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
