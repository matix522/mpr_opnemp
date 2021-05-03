#!/bin/bash

size_list=(2 20 200 2000 20000 200000 2000000 20000000 200000000)

./compile.sh

schedule_list=("guided,16")

threads_list=(1 4 6 8 12 24)

echo -e "iters\tthreads\tscheduling\ttab_size\ttime" > seq_results_laptop.tsv
for size in  "${size_list[@]}"; do
    for iter in {0..9}; do
        echo -e "${iter}\t1\t${non_applicable}\t${size}\t"`(time ./random_seq ${size}) 2>&1 | grep "real" | awk '{print $2}'` >> seq_results_laptop.tsv
    done
done

echo -e "iters\tthreads\tscheduling\ttab_size\ttime" > par_results_laptop.tsv
for threads in "${threads_list[@]}"; do
    for schedule in "${schedule_list[@]}"; do
        for size in  "${size_list[@]}"; do
            for iter in {0..9}; do
                export OMP_SCHEDULE=${schedule}
                export OMP_NUM_THREADS=${threads}
                echo -e "${iter}\t${threads}\t${OMP_SCHEDULE}\t${size}\t"`(time ./random ${size}) 2>&1 | grep "real" | awk '{print $2}'` >> par_results_laptop.tsv
            done
        done
    done
done
