#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <thread>
#include <future>

using namespace std;

std::mutex clusterMtx;

vector<pair<int, int>> clusterKernel(vector<int> dataIndices, std::shared_ptr<FeatureMatrix> _data, vector<int> range, vector<int> centroidIndices, map<int, bool> isCentroid)
{
    auto data = *_data;
    vector<pair<int, int>> localThreadResults;

    // iterate data within range
    for (int x = 0; x < range.size(); x++) // range goes from 0th element of dataIndicies to nth (aka is a local index)
    {
        int i = range[x];
        bool isDataPointACentroid = isCentroid[i];
        if (isDataPointACentroid == true)
            continue; // skip calculating optimal membership for centroids as they are not members of any cluster

        int dataIndex = dataIndices[i];
        auto currentFeatureData = data[dataIndex];
        int nearestCentroidIndex = -1; // This is a poor choice FIXME
        long long nearestDistance = LLONG_MAX;

        // find the closest centroid, if the current data point index means it is a centroid then ?
        for (int c = 0; c < centroidIndices.size(); c++)
        {
            const int centroidLocalIndex = centroidIndices[c];
            const int centroidGlobalIndex = dataIndices[centroidLocalIndex];
            auto currentCentroidData = data[centroidGlobalIndex];

            // calculate pairwise hamming distance
            long long distance = hammingDistance(currentCentroidData, currentFeatureData);

            if (distance < nearestDistance)
            {
                nearestCentroidIndex = c;
                nearestDistance = distance;
            }
        }

        if (nearestCentroidIndex == -1)
        {
            cout << "bad math" << endl;
            exit(1);
        }

        localThreadResults.push_back(make_pair(nearestCentroidIndex, i));
    }
    return localThreadResults;
}

map<int, vector<int>> optimiseClusterMembership(vector<int> dataIndices, std::shared_ptr<FeatureMatrix> _data, vector<int> centroidSeedIndices, int processor_count)
{
    auto data = *_data;
    map<int, bool> isCentroid = {};
    map<int, vector<int>> clusterMembership = {};

    const int dataRowCount = dataIndices.size();
    const int threadPool = processor_count;

    auto range = getRange(dataRowCount);
    auto distributedTasks = distributeTasks(range, threadPool);

    // for each centroid
    for (int c = 0; c < centroidSeedIndices.size(); c++)
    {
        const int cCentroidLocalIndex = centroidSeedIndices[c]; // this is local index in dataIndicies
        isCentroid[cCentroidLocalIndex] = true;
        clusterMembership[c] = {};
    }

    vector<pair<int, int>> threadResults = {};
    vector<std::future<std::vector<std::pair<int, int>>>> futures = {};

    for (int i = 0; i < threadPool; i++)
    {
        vector<int> threadTask = distributedTasks[i];
        auto indexClone = dataIndices;
        auto centroidSeedIndicesClone = centroidSeedIndices;
    
        auto future = std::async(std::launch::async, [_data, &isCentroid, threadTask = std::move(threadTask), indexClone = std::move(indexClone), centroidSeedIndices = std::move(centroidSeedIndicesClone)]()
                                 { return clusterKernel(indexClone, _data, threadTask, centroidSeedIndices, isCentroid); });

        futures.emplace_back(std::move(future));
    }

    for (int i = 0; i < futures.size(); i++)
    {
        auto data = futures[i].get();
        for (int i = 0; i < data.size(); i++)
        {
            // nearestCentroidIndex (0,...,n) , local index to dataindicies
            clusterMembership[data[i].first].push_back(data[i].second);
        }
    }
    return clusterMembership;
}
