#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
using namespace std;

map<int, vector<int>> kmedoids(std::shared_ptr<FeatureMatrix> _data, vector<int> *_indices, int k, int processor_count, vector<int> seeds) {
    auto data = *_data;
    auto indices = *_indices;
    vector<int> centroids = seedCentroids(indices, _data, k, seeds);
    cout << "a" << endl;
    // begin fitting
    long long bestCost = LLONG_MAX;
    auto bestMembership = optimiseClusterMembership(indices, _data, centroids, processor_count);
    cout << "b" << endl;
    bool escape = false;

    int iteration = 0;
    while (escape == false)
    {
        auto optimalSelectionResults = optimiseCentroidSelectionAndComputeClusterCost(indices, _data, centroids, bestMembership, processor_count);
        cout << "c" << endl;
        auto cost = get<0>(optimalSelectionResults);
                cout << "c1" << endl;

        auto clusterMembership = get<1>(optimalSelectionResults);
                cout << "c2" << endl;

        auto newCentroids = get<2>(optimalSelectionResults);
                cout << "c3" << endl;

                centroidPrinter(newCentroids);

        // centroids = newCentroids; // getClusterKeys(clusterMembership);
        clusterMembership = optimiseClusterMembership(indices, _data, newCentroids, processor_count);
       
        cout << "d" << endl;
        if (cost < bestCost)
        {
            cout << "e" << endl;
            cout << "Cost improving (currentCost, oldCost)" << cost << " , " << bestCost << endl;
            clusterMembershipPrinter(clusterMembership);
            bestCost = cost;
            bestMembership = clusterMembership;
            centroids = newCentroids;
        }
        else
        {
            escape = true;
        }
        iteration++;
    }

    // centroids = getClusterKeys(bestMembership);
    centroidPrinter(centroids);

    return bestMembership;
}