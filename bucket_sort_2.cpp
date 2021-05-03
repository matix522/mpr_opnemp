#include <iostream>
#include <cstdint>
#include <vector>
#include <unistd.h>
#include <omp.h>
#include <set>
#include <algorithm>
#include <cassert>
#include <boost/container/small_vector.hpp>

constexpr std::size_t BUCKET_RANGE = 256;

 // THIS WILL ALLOW US TO USE ONE MAIN ALLOCATION FOR ALL BUCKETS AND THEN MAKE ADDITIONAL ALLOCATION WEN NEEDED.
using Bucket = boost::container::small_vector<int32_t, BUCKET_RANGE * 2>; 
struct Timestamps {
    double program_start;                       // a_start, e_start
    double randomization_finished;              // a_end, b_start
    double from_array_into_buckets_finished;    // b_end, c_start
    double sorting_buckets_finished;            // c_end, d_start
    double from_buckets_into_array_finished;    // d_end, e_end 
};
Timestamps timestamps;

std::vector<Bucket> preallocate_buckets(std::size_t bucket_count)
{
    std::vector<Bucket> buckets;
    buckets.resize(bucket_count);
    for (auto& bucket : buckets) {
        bucket.reserve(2 * BUCKET_RANGE);
    }
    return buckets;
}
std::vector<omp_lock_t> make_locks_for_buckets(std::size_t bucket_count)
{
    std::vector<omp_lock_t> bucket_locks;
    bucket_locks.resize(bucket_count);
    for (auto& lock : bucket_locks) {
        omp_init_lock(&lock);    
    }
    return bucket_locks;
}
std::size_t sort_and_count_buckets(std::vector<Bucket>& buckets, std::size_t start_iter, std::size_t end_iter)
{
    std::size_t bucket_element_count = 0;
    
    for (std::size_t i = start_iter; i < end_iter; i++) {
        auto& bucket = buckets[i];
        std::sort(bucket.begin(), bucket.end());
        bucket_element_count += bucket.size();
    }
    return bucket_element_count;
}

void aggregate_counts(std::vector<std::size_t>& count_vector) {
    #pragma omp single
    {
        std::size_t running_sum = 0;
        for (std::size_t& count : count_vector) {
            count += running_sum;
            running_sum = count;
        }
    }
}

void from_buckets_insert_into_array(std::vector<int32_t>& numbers, 
                                    const std::vector<Bucket>& buckets,
                                    const std::size_t bucket_iter_start, 
                                    const std::size_t bucket_iter_end,
                                    const std::size_t start_index,
                                    const std::size_t end_index)
{
    std::size_t index = start_index;

    for (std::size_t i = bucket_iter_start; i < bucket_iter_end; i++) {
        auto& bucket = buckets[i];
        for (int32_t value : bucket){
            numbers[index++] = value;
        }
    }
    assert(index == end_index);
}

void from_array_insert_into_buckets(std::vector<Bucket>& buckets, 
                                    std::vector<omp_lock_t>& locks,
                                    const std::vector<int32_t>& numbers, 
                                    const std::size_t iter_start, 
                                    const std::size_t iter_end,
                                    const int32_t minimum) 
{
    for (std::size_t i = iter_start; i < iter_end; i++) {
        const int32_t value = numbers[i];
        const std::size_t bucket_index = (value - minimum) / BUCKET_RANGE;
        omp_set_lock(&locks[bucket_index]);
        {
            buckets[bucket_index].push_back(value);
        }
        omp_unset_lock(&locks[bucket_index]);

    }
}

std::vector<int32_t> prepare_random_numbers(const std::size_t size, const int32_t minimum, const int32_t maximum)
{
    std::vector<int32_t> numbers;
    numbers.resize(size);

    uint32_t thread_seed=0;
    std::size_t i = 0;

    #pragma omp parallel shared(numbers, size, maximum, minimum) private(i, thread_seed) default(none) 
    {
        thread_seed = time(NULL) ^ getpid() ^ pthread_self();
        #pragma omp for schedule(static)
        for (i = 0; i < size; i++) {
            numbers[i] = rand_r(&thread_seed) % (maximum - minimum) + minimum;
        } 
    }

    return numbers;
}

int main(int argc, char** argv, char** env) {
    
    if (argc < 2) {
        std::cerr << "provide size of problem" << std::endl;
        return 1;
    }
    const std::size_t size = std::stol(argv[1]);

    const int32_t minimum = 0;
    const int32_t maximum = size;
    const int32_t value_difference = maximum - minimum;

    timestamps.program_start = omp_get_wtime();

    std::vector<int32_t> numbers = prepare_random_numbers(size, minimum, maximum);
    timestamps.randomization_finished = omp_get_wtime();


    const std::size_t bucket_count = (value_difference) / BUCKET_RANGE + 1;
    std::vector<Bucket> buckets = preallocate_buckets(bucket_count);
    std::vector<omp_lock_t> bucket_locks = make_locks_for_buckets(bucket_count);

    std::vector<std::size_t> bucket_element_counts;

    #pragma omp parallel shared(numbers, size, minimum, maximum, value_difference, bucket_element_counts, timestamps, buckets, bucket_locks, bucket_count) default(none)
    {
        const uint32_t thread_count = omp_get_num_threads();
        const uint32_t thread_id = omp_get_thread_num();

        #pragma omp single
        {
            bucket_element_counts.resize(thread_count + 1);
        }
        // CALCULATE ALL CONSTANTS NEEDED LATER
        const std::size_t iter_start = (thread_id * size) / thread_count;
        const std::size_t iter_end = ((thread_id + 1) * size) / thread_count;

        const std::size_t bucket_iter_start = (thread_id * bucket_count) / thread_count;
        const std::size_t bucket_iter_end = ((thread_id + 1) * bucket_count) / thread_count;


        from_array_insert_into_buckets(buckets, bucket_locks, numbers, iter_start, iter_end, minimum);

        #pragma omp barrier     // THIS BARRIER IS REQUIRED EVEN WITHOUT OMP_GET_WTIME
        #pragma omp master
        {
            timestamps.from_array_into_buckets_finished = omp_get_wtime();
        }
        bucket_element_counts[thread_id + 1] = sort_and_count_buckets(buckets, bucket_iter_start, bucket_iter_end);

        #pragma omp barrier     // THIS BARRIER IS REQUIRED EVEN WITHOUT OMP_GET_WTIME
        #pragma omp master
        {
            timestamps.sorting_buckets_finished = omp_get_wtime();
        }
        aggregate_counts(bucket_element_counts);
        assert(bucket_element_counts[thread_count] == size);
        from_buckets_insert_into_array(numbers, buckets, bucket_iter_start, bucket_iter_end, bucket_element_counts[thread_id], bucket_element_counts[thread_id + 1]);
    }
    timestamps.from_buckets_into_array_finished = omp_get_wtime();

    assert(std::is_sorted(numbers.begin(), numbers.end()));

    const double a = timestamps.randomization_finished - timestamps.program_start;
    const double b = timestamps.from_array_into_buckets_finished - timestamps.randomization_finished;
    const double c = timestamps.sorting_buckets_finished - timestamps.from_array_into_buckets_finished;
    const double d = timestamps.from_buckets_into_array_finished - timestamps.sorting_buckets_finished;
    const double e = timestamps.from_buckets_into_array_finished - timestamps.program_start;

    printf("a\t\tb\t\tc\t\td\t\te\n");
    printf("%fs\t%fs\t%fs\t%fs\t%fs\n", a, b, c, d, e);

    return 0;
}