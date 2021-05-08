#!/bin/bash

size=20000000
bucket_size_list=(64 128 256 512 1024 2048 4096 8196 20480 40960 81960 204800 409600 819600)

./compile.sh

export OMP_NUM_THREADS=1
echo -e "iters\tproblem_size\tbucket_range\ttime_a\ttime_b\ttime_c\ttime_d\ttime_e" > bucket_size_results_algorithm_1.tsv
for bucket in "${bucket_size_list[@]}"; do
    for iter in {0..4}; do
        echo -en "${iter}\t${size}\t${bucket}\t" >> bucket_size_results_algorithm_1.tsv
        ./bucket_sort_1 ${size} ${bucket} >> bucket_size_results_algorithm_1.tsv
    done
done

export OMP_NUM_THREADS=1
echo -e "iters\tproblem_size\tbucket_range\ttime_a\ttime_b\ttime_c\ttime_d\ttime_e" > bucket_size_results_algorithm_2.tsv
for bucket in "${bucket_size_list[@]}"; do
    for iter in {0..4}; do
        echo -en "${iter}\t${size}\t${bucket}\t" >> bucket_size_results_algorithm_2.tsv
        ./bucket_sort_2 ${size} ${bucket} >> bucket_size_results_algorithm_2.tsv
    done
done



