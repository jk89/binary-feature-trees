#include <gtest/gtest.h>
#include <climits>


#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>
#include <stdlib.h> /* atoi */
#include <thread>
#include <random>
#include <iterator>
#include <algorithm>
#include <chrono>
#include "../src/models.cpp"
#include "../src/utils.cpp"
#include "../src/algorithms/kpp.cpp"
#include "../src/algorithms/cluster.cpp"
#include "../src/algorithms/optimise.cpp"
#include "../src/algorithms/kmedoids.cpp"
#include "../src/algorithms/voctrain.cpp"
#include "../src/algorithms/voctree.cpp"
#include <time.h>

using namespace std;
using namespace boost::filesystem;
using namespace std::chrono;

namespace fs = boost::filesystem;

int add(int a, int b){
    return a + b;
}

TEST(NumberCmpTest2, ShouldPass){
    ASSERT_EQ(3, add(1,2));
}

TEST(NumberCmpTest3, ShouldFail){
    ASSERT_EQ(INT_MAX, add(INT_MAX, 1));
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}