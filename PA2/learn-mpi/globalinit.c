   MPI_Init(&argc, &argv);
   int procno, nprocs;
   MPI_Comm comm = MPI_COMM_WORLD;
   MPI_Comm_rank(MPI_COMM_WORLD, &procno);
   MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
