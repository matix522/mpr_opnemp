#!/bin/bash

size=50000000
bucket_size=524288

./compile.sh

echo -e "iters\tproblem_size\tbucket_range\tthreads\ttime_a\ttime_b\ttime_c\ttime_d\ttime_e" > speedups_strong_$size.${bucket_size}_1.tsv
for threads in {1..8}; do
    export OMP_NUM_THREADS=${threads}
    for iter in {0..4}; do
        echo -en "${iter}\t${size}\t${bucket_size}\t${threads}\t" >> speedups_strong_$size.${bucket_size}_1.tsv
        ./bucket_sort_1 ${size} ${bucket_size} >> speedups_strong_$size.${bucket_size}_1.tsv
    done
done

bucket_size=524288

echo -e "iters\tproblem_size\tbucket_range\tthreads\ttime_a\ttime_b\ttime_c\ttime_d\ttime_e" > speedups_strong_$size.${bucket_size}_2.tsv
for threads in {1..8}; do
    export OMP_NUM_THREADS=${threads}
    for iter in {0..4}; do
        echo -en "${iter}\t${size}\t${bucket_size}\t${threads}\t" >> speedups_strong_$size.${bucket_size}_2.tsv
        ./bucket_sort_2 ${size} ${bucket_size} >> speedups_strong_$size.${bucket_size}_2.tsv
    done
done



