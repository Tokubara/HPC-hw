BEGIN {
	valid_row_num=0	
}
{
	if(NF==3) {
		valid_row_num+=1
		N[valid_row_num]=$1
		n[valid_row_num]=$2
		runtime[valid_row_num]=$3
	} else if(NF!=0) {
		printf "error line: %s",$0
		exit 1
	}
}
END {
	for(i=1;i<=valid_row_num;i++) {
		printf "N=%d,P=%d,runtime=%fms,speedup=%f\n",N[i],n[i]/N[i],runtime[i],runtime[1]/runtime[i]
	}
}
