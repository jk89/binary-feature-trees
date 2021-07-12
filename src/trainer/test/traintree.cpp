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

const auto processor_count = std::thread::hardware_concurrency();

TEST(ShouldBuildSmallestModel, ShouldPass) {
    // remove model files
    
    string modelName = "smallest";

    //removeFileIfExist
    string vocTree = "data/" + modelName + "_voctree.json";
    string vocModel = "data/" + modelName + "_voccompute.json";
    removeFileIfExist(vocTree); removeFileIfExist(vocModel);

    trainModel(modelName, 8, 12);
    trainModelToComputeModel(modelName);
    auto model = getComputeModelByName(modelName);
    ASSERT_EQ(model.feature_id, -1);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}