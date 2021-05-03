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

std::vector<Bucket> preallocate_buckets(std::size_t per_thread_bucket_count)
{
    std::vector<Bucket> buckets;
    buckets.resize(per_thread_bucket_count);
    for (auto& bucket : buckets) {
        // AS WE DONT KNOW THE DISTRIBUTION OF OUR INTS WE CAN ASSUME UNIFORM DISTRIBUTION
        // PREALOCATING EXPECTED MEAN VALUE IS NOT THE BEST IDEA AS WE CAN EASLIY CASUE REALOCATION 
        // BY ADDING BUCKET_RANGE-TH ELEMENT. 
        // MAKING THE PREALOCATED VALUE DOUBLE OF WHAT WE EXPECT MOSTLY SOLVES THAT PROBLEM 
        bucket.reserve(2 * BUCKET_RANGE);
    }
    return buckets;
}

std::size_t sort_and_count_buckets(std::vector<Bucket>& buckets)
{
    std::size_t thread_bucket_element_count = 0;
    for (auto& bucket : buckets) {
        std::sort(bucket.begin(), bucket.end());
        thread_bucket_element_count += bucket.size();
    }
    return thread_bucket_element_count;
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
                                    const std::size_t start_index,
                                    const std::size_t end_index)
{
    std::size_t index = start_index;

    for (auto& bucket : buckets) {
        for (int32_t value : bucket){
            numbers[index++] = value;
        }
    }
    assert(index == end_index);
}

void from_array_insert_into_buckets(std::vector<Bucket>& buckets, 
                                    const std::vector<int32_t>& numbers, 
                                    const std::size_t iter_start, 
                                    const int32_t thread_minimum, 
                                    const int32_t thread_maximum) 
{
    // A COPY OF THE LOOP IS USED TO AVOID CHECKS OR MODULO OPERATIONS
    for (std::size_t i = iter_start; i < numbers.size(); i++) {
        const int32_t value = numbers[i];
        if (value < thread_minimum || value >= thread_maximum) {
            continue;
        }
        const std::size_t bucket_index = (value - thread_minimum) / BUCKET_RANGE;
        buckets[bucket_index].push_back(value);
    }
    for (std::size_t i = 0; i < iter_start; i++) {
        const int32_t value = numbers[i];
        if (value < thread_minimum || value >= thread_maximum) {
            continue;
        }
        const std::size_t bucket_index = (value - thread_minimum) / BUCKET_RANGE;
        buckets[bucket_index].push_back(value);
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

    std::vector<std::size_t> bucket_element_counts;

    #pragma omp parallel shared(numbers, size, minimum, maximum, value_difference, bucket_element_counts, timestamps) default(none)
    {
        const uint32_t thread_count = omp_get_num_threads();
        const uint32_t thread_id = omp_get_thread_num();

        #pragma omp single
        {
            bucket_element_counts.resize(thread_count + 1);
        }
        // CALCULATE ALL CONSTANTS NEEDED LATER
        const std::size_t iter_start = (thread_id * size) / thread_count;

        const int32_t thread_minimum = (thread_id * value_difference) / thread_count + minimum;
        const int32_t thread_maximum = ((thread_id + 1) * value_difference) / thread_count + minimum;

        const std::size_t per_thread_bucket_count = (thread_maximum - thread_minimum) / BUCKET_RANGE + 1;

        std::vector<Bucket> buckets = preallocate_buckets(per_thread_bucket_count);

        from_array_insert_into_buckets(buckets, numbers, iter_start, thread_minimum, thread_maximum);

        #pragma omp barrier
        #pragma omp master
        {
            timestamps.from_array_into_buckets_finished = omp_get_wtime();
        }
        bucket_element_counts[thread_id + 1] = sort_and_count_buckets(buckets);

        #pragma omp barrier     // THIS BARRIER IS REQUIRED EVEN WITHOUT OMP_GET_WTIME
        #pragma omp master
        {
            timestamps.sorting_buckets_finished = omp_get_wtime();
        }
        aggregate_counts(bucket_element_counts);
        assert(bucket_element_counts[thread_count] == size);
        from_buckets_insert_into_array(numbers, buckets, bucket_element_counts[thread_id], bucket_element_counts[thread_id + 1]);
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