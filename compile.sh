#!/bin/bash
g++ -std=c++11 bucket_sort_1.cpp -o bucket_sort_1 -O3 -fopenmp -march=native -mtune=native -Wall -Wpedantic
g++ -std=c++11 bucket_sort_2.cpp -o bucket_sort_2 -O3 -fopenmp -march=native -mtune=native -Wall -Wpedantic 