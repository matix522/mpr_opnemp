#!/bin/bash
g++ -std=c++11 random.cpp -o random -O3 -fopenmp
g++ -std=c++11 random_seq.cpp -o random_seq -O3
g++ -std=c++11 bucket_sort_1.cpp -o bucket_sort_1 -O3 -fopenmp -Wall -Wpedantic
g++ -std=c++11 bucket_sort_2.cpp -o bucket_sort_2 -O3 -fopenmp -Wall -Wpedantic 
