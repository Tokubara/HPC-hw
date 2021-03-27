{
	N[NR]=$1
	n[NR]=$2
	runtime[NR]=$3
}
END {
	for(i=1;i<=NR;i++) {
		printf "N=%d,P=%d,runtime=%fms,ratio=%f\n",N[i],n[i]/N[i],runtime[i],runtime[i]/runtime[1]
	}
}
