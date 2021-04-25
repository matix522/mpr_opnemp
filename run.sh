#!/bin/bash

size_list=(2 20 200 2000 20000 200000 2000000 20000000 200000000)

./compile.sh

schedule_list=("dynamic" "static" "dynamic,1" "static,1" "dynamic,4" "static,4" "dynamic,64" "static,64" "dynamic,4096" "static,4096" )

echo -e "iters\tscheduling\ttab_size\ttime" > seq_results.tsv
for size in  "${size_list[@]}"; do
    for iter in {0..9}; do
        echo -e "${iter}\t${non_applicable}\t${size}\t"`(time ./random_seq ${size}) 2>&1 | grep "real" | awk '{print $2}'` >> seq_results.tsv
    done
done


export OMP_NUM_THREADS=1
echo -e "iters\tscheduling\ttab_size\ttime" > par_1_thread_results.tsv
for size in  "${size_list[@]}"; do
    for iter in {0..9}; do
        echo -e "${iter}\t${non_applicable}\t${size}\t"`(time ./random_seq ${size}) 2>&1 | grep "real" | awk '{print $2}'` >> par_1_thread_results.tsv
    done
done

export OMP_NUM_THREADS=4
echo -e "iters\tscheduling\ttab_size\ttime" > par_results.tsv
for schedule in "${schedule_list[@]}"; do
    for size in  "${size_list[@]}"; do
        for iter in {0..9}; do
            export OMP_SCHEDULE=${schedule}
            echo -e "${iter}\t${OMP_SCHEDULE}\t${size}\t"`(time ./random ${size}) 2>&1 | grep "real" | awk '{print $2}'` >> par_results.tsv
        done
    done
done

