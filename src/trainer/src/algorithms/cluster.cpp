#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <thread>
#include <future>

using namespace std;

std::mutex clusterMtx;

vector<pair<int, int>> clusterKernel(vector<int> *_dataIndices, std::shared_ptr<FeatureMatrix> _data, vector<int> range, vector<int> centroidIndices, map<int, bool> isCentroid)
{
    auto dataIndices = *_dataIndices;
    auto data = *_data;
    vector<pair<int, int>> localThreadResults;

    // iterate data within range
    for (int x = 0; x < range.size(); x++)
    {
        int i = range[x];
        bool isDataPointACentroid = isCentroid[i];
        if (isDataPointACentroid == true)
            continue; // skip calculating optimal membership for centroids as they are not members of any cluster

        // convert range index to data index
        int dataIndex = dataIndices[i];
        auto currentFeatureData = data[dataIndex];
        int nearestCentroidIndex = -1; // This is a poor choice FIXME
        long long nearestDistance = LLONG_MAX;

        // find the closest centroid, if the current data point index means it is a centroid then ?
        for (int c = 0; c < centroidIndices.size(); c++)
        {
            const int centroidIndex = centroidIndices[c];
            auto currentCentroidData = data[centroidIndex];

            // calculate pairwise hamming distance
            long long distance = hammingDistance(currentCentroidData, currentFeatureData);

            if (distance < nearestDistance)
            {
                nearestCentroidIndex = centroidIndex;
                nearestDistance = distance;
            }
        }

        if (nearestCentroidIndex == -1)
        {
            cout << "bad math" << endl;
            exit(1);
        }

        localThreadResults.push_back(make_pair(nearestCentroidIndex, dataIndex));
    }
    return localThreadResults;
}


map<int, vector<int>> optimiseClusterMembership(vector<int> *_dataIndices, std::shared_ptr<FeatureMatrix> _data, vector<int> &centroidSeedIndices, int processor_count)
{
    auto dataIndices = *_dataIndices;
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
        const int cCentroidIndex = centroidSeedIndices[c];
        isCentroid[cCentroidIndex] = true;
        clusterMembership[cCentroidIndex] = {};
    }

    vector<pair<int, int>> threadResults = {};
    vector<std::future<std::vector<std::pair<int, int>>>> futures = {};

    for (int i = 0; i < threadPool; i++)
    {
        vector<int> threadTask = distributedTasks[i];
        auto future = std::async(std::launch::async, [&_dataIndices, &_data, &centroidSeedIndices, &isCentroid, threadTask = std::move(threadTask)]()
                                 {
                                     return clusterKernel(_dataIndices, _data, threadTask, centroidSeedIndices, isCentroid);
                                 });
        futures.emplace_back(std::move(future));
    }

    for (int i = 0; i < futures.size(); i++)
    {
        auto data = futures[i].get();
        for (int i = 0; i < data.size(); i++)
        {
            clusterMembership[data[i].first].push_back(data[i].second);
        }
    }
    return clusterMembership;
}
