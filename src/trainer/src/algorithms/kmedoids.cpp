#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
using namespace std;

map<int, vector<int>> kmedoids(std::shared_ptr<FeatureMatrix> _data, vector<int> *_indices, int k, int processor_count, vector<int> seeds) {
    auto data = *_data;
    auto indices = *_indices;
    cout << "details of m rows" << data.size() << " cols " << data[0].size() << " tyoe " << endl; // << data.type()
    cout << "Seeding clusters:" << endl;
    vector<int> centroids = seedCentroids(_data, k, seeds);
    cout << "Centroids: " << endl;
    for (auto i = centroids.begin(); i != centroids.end(); ++i)
    {
        std::cout << *i << ", ";
    }
    cout << endl;

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
        cout << "centroids2:" << centroids.size() << endl;
        for (auto i = centroids.begin(); i != centroids.end(); ++i)
        {
            cout << *i << ", ";
        }
        cout << endl;
        cout << " about to opt again" << endl;
        clusterMembership = optimiseClusterMembership(_indices, _data, centroids, processor_count);
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

    centroids = getClusterKeys(bestMembership);
    centroidPrinter(centroids);

    return bestMembership;
}