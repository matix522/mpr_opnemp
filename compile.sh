#!/bin/bash
g++ -std=c++11 random.cpp -o random -O3 -fopenmp
g++ -std=c++11 random_seq.cpp -o random_seq -O3