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

    uint32_t thread_seed=0;
    std::size_t i = 0;

    srand(time(NULL));

    for (i = 0; i < size; i++) 
    {
        numbers[i] = rand() % size;
    } 
    return 0;
}