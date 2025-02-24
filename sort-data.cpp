#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <random>
#include <string>

namespace {
    void SortData(int iterations, std::vector<int>& data) {
        for (int i = 0; i < iterations; ++i) {
            std::sort(data.begin(), data.end());
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <iterations> <block_size>\n";
        return 1;
    }

    int iterations = argc > 1 ? std::stoi(argv[1]) : 0;
    int block_size = std::stoi(argv[2]);

    std::vector<int> data(block_size);
    std::iota(data.begin(), data.end(), 0);

    std::random_device random_device;
    std::mt19937 generator(random_device());  // Используем random_device для генератора случайных чисел

    std::shuffle(data.begin(), data.end(), generator);

    std::cout << "Generated data:\n";
    for (const auto& num : data) {
        std::cout << num << " ";
    }
    std::cout << "\n";

    SortData(iterations, data);

    std::cout << "Sorted data:\n";
    for (const auto& num : data) {
        std::cout << num << " ";
    }
    std::cout << "\n";

    return 0;
}
