#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
using namespace std;

map<int, vector<int>> kmedoids(cv::Mat &data, vector<int> &indices, int k, int processor_count, vector<int> seeds) {
    cout << "details of m rows" << data.rows << " cols " << data.cols << " tyoe " << data.type() << endl;
    cout << "first row: " << cv::format(data.row(0), cv::Formatter::FMT_PYTHON) << endl;
    cout << "second row: " << cv::format(data.row(1), cv::Formatter::FMT_PYTHON) << endl;
    cout << "Seeding clusters:" << endl;
    vector<int> centroids = seedClusters(data, k, seeds);
    cout << "Centroids: " << endl;
    for (auto i = centroids.begin(); i != centroids.end(); ++i)
    {
        std::cout << *i << ", ";
    }
    cout << endl;

    // begin fitting
    long long bestCost = LLONG_MAX;
    auto bestMembership = optimiseClusterMembership(indices, data, centroids, processor_count);
    bool escape = false;

    int iteration = 0;
    while (escape == false)
    {
        auto optimalSelectionResults = optimiseCentroidSelectionAndComputeCost(data, bestMembership, processor_count);
        auto cost = optimalSelectionResults.first;
        auto clusterMembership = optimalSelectionResults.second;
        centroids = getClusterKeys(clusterMembership);
        cout << "centroids2:" << centroids.size() << endl;
        for (auto i = centroids.begin(); i != centroids.end(); ++i)
        {
            cout << *i << ", ";
        }
        cout << endl;
        cout << " about to opt again" << endl;
        clusterMembership = optimiseClusterMembership(indices, data, centroids, processor_count);
        if (cost < bestCost)
        {
            cout << "Optimsiation improving (currentCost, oldCost)" << cost << " , " << bestCost << endl;
            clusterMembershipPrinter(clusterMembership);
            bestCost = cost;
            bestMembership = clusterMembership;
        }
        else
        {
            escape = true;
        }
        iteration++;
    }
    return bestMembership;
}

    