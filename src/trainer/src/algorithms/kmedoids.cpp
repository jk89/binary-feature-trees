#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
using namespace std;

map<int, vector<int>> kmedoids(std::shared_ptr<FeatureMatrix> _data, vector<int> *_indices, int k, int processor_count, vector<int> seeds) {
    auto data = *_data;
    auto indices = *_indices;
    vector<int> centroids = seedCentroids(_data, k, seeds);

    // begin fitting
    long long bestCost = LLONG_MAX;
    auto bestMembership = optimiseClusterMembership(_indices, _data, centroids, processor_count);
    bool escape = false;

    int iteration = 0;
    while (escape == false)
    {
        auto optimalSelectionResults = optimiseCentroidSelectionAndComputeClusterCost(_data, bestMembership, processor_count);
        auto cost = optimalSelectionResults.first;
        auto clusterMembership = optimalSelectionResults.second;
        centroids = getClusterKeys(clusterMembership);
        clusterMembership = optimiseClusterMembership(_indices, _data, centroids, processor_count);
        if (cost < bestCost)
        {
            cout << "Cost improving (currentCost, oldCost)" << cost << " , " << bestCost << endl;
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

    centroids = getClusterKeys(bestMembership);
    centroidPrinter(centroids);

    return bestMembership;
}