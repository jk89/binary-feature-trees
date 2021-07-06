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
#include "algorithms/futuretesting.cpp"
#include "algorithms/kpp.cpp"
#include "algorithms/cluster.cpp"
#include "algorithms/optimise.cpp"
#include "algorithms/kmedoids.cpp"
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

// (clusterKernel) cv::Mat data, cv::Mat currentCentroidData, ConcurrentIndexRange range, vector<int> centroidIndices, cv::Mat distances

// dep'd as arg cv::Mat currentCentroidData,

int main(int argc, char **argv)
{
    /*cv::Mat data;
    data =  load_data(filename);
    writeFeaturesToFile("smallest_features.yml", data);
    exit(0);*/
    //struct timespec delta = {10 /*secs*/, 135 /*nanosecs*/};
    //while (nanosleep(&delta, &delta));
    //cout << "slept" << endl;

    trainModel("small");
    /* cv::Mat data;
    data =  readFeaturesFromFile("data/features.yml");//  load_data(filename); // 
    // writeFeaturesToFile("features.yml", data);
    vector<int> indices;
    for (int i = 0; i < data.rows; i++)
    {
        indices.push_back(i);
    }
    // auto _data = &data;
    map<int, vector<int>> kmedoidsClusterMembership = kmedoids(&data, &indices, 8, processor_count, {0});

    cout << "ALL DONE. Best membership:" << endl;
    // clusterMembershipPrinter(kmedoidsClusterMembership);

    cout << "Best centroids:" << endl;
    // centroidPrinter(getClusterKeys(kmedoidsClusterMembership));*/
}
