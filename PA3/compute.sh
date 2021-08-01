paste -d ' ' result_ref_log.txt result_mine_32_log.txt |  awk -F ' ' '{OFS=FS=" ";speedup=$1/$2;print speedup}'
