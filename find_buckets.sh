#!/bin/bash

size=50000000
bucket_size_list=(2048)

./compile.sh

export OMP_NUM_THREADS=1
echo -e "iters\tproblem_size\tbucket_range\ttime_a\ttime_b\ttime_c\ttime_d\ttime_e" > bucket_size_results_algorithm_1_pk_$size.2048.tsv
for bucket in "${bucket_size_list[@]}"; do
    for iter in {0..4}; do
        echo -en "${iter}\t${size}\t${bucket}\t" >> bucket_size_results_algorithm_1_pk_$size.2048.tsv
        ./bucket_sort_1 ${size} ${bucket} >> bucket_size_results_algorithm_1_pk_$size.2048.tsv
    done
done

export OMP_NUM_THREADS=1
echo -e "iters\tproblem_size\tbucket_range\ttime_a\ttime_b\ttime_c\ttime_d\ttime_e" > bucket_size_results_algorithm_2_pk_$size.2048.tsv
for bucket in "${bucket_size_list[@]}"; do
    for iter in {0..4}; do
        echo -en "${iter}\t${size}\t${bucket}\t" >> bucket_size_results_algorithm_2_pk_$size.2048.tsv
        ./bucket_sort_2 ${size} ${bucket} >> bucket_size_results_algorithm_2_pk_$size.2048.tsv
    done
done



