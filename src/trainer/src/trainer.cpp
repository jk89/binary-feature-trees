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
#include "models.cpp"
#include "utils.cpp"
#include "algorithms/kpp.cpp"
#include "algorithms/cluster.cpp"
#include "algorithms/optimise.cpp"
#include "algorithms/kmedoids.cpp"
#include "algorithms/voctrain.cpp"
#include "algorithms/voctree.cpp"

#include <time.h>
// std::async, std::future

// using namespace cv;
using namespace std;
using namespace boost::filesystem;
using namespace std::chrono;

namespace fs = boost::filesystem;

char *filename = "./data/ORBvoc.txt";
const auto processor_count = std::thread::hardware_concurrency();

int main(int argc, char **argv)
{
    // trainModel("smallest", 8, processor_count);
    //kmedoids(sData, &origIndicies, 8, 1, {0}); exit(1);
}
