#include <iostream>
#include <cstdint>
#include <vector>
#include <unistd.h>

int main(int argc, char** argv, char** env) {
    
    if (argc < 2) {
        std::cerr << "provide size of problem" << std::endl;
        return 1;
    }
    const std::size_t size = std::stol(argv[1]);

    std::vector<int32_t> numbers;
    numbers.resize(size);

    uint32_t thread_seed = time(NULL) ^ getpid() ^ pthread_self();
    for (std::size_t i = 0; i < size; i++) {
        numbers[i] = rand_r(&thread_seed) % size;
    } 
    return 0;
}