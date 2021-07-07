#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <thread>
#include <future>

using namespace std;

std::mutex clusterMtx;

vector<pair<int, int>> clusterKernel(vector<int> dataIndices, std::shared_ptr<FeatureMatrix> _data, vector<int> range, vector<int> centroidIndices, map<int, bool> isCentroid)
{
    // auto dataIndices = *_dataIndices;
    auto data = *_data;
    vector<pair<int, int>> localThreadResults;

    // iterate data within range
    for (int x = 0; x < range.size(); x++) // range goes from 0th element of dataIndicies to nth (aka is a local index)
    {
        cout << "XClusterKernel:x|1~" << x << endl;
        int i = range[x];
                cout << "XClusterKernel:x|2~" << x << endl;

        // should the isCentroid check be global? aka lookup dataIndicies here FIXME
        bool isDataPointACentroid = isCentroid[i];
                cout << "XClusterKernel:x|3~" << x << endl;

        if (isDataPointACentroid == true)
            continue; // skip calculating optimal membership for centroids as they are not members of any cluster

        // convert range index to data index
                cout << "XClusterKernel:x|4~" << x << endl;

        int dataIndex = dataIndices[i];
                cout << "XClusterKernel:x|5~" << x << endl;

        auto currentFeatureData = data[dataIndex];
                cout << "XClusterKernel:x|6~" << x << endl;

        centroidPrinter(centroidIndices);

        cout << endl;

        cout << dataIndices.size() << endl;

        int nearestCentroidIndex = -1; // This is a poor choice FIXME
        long long nearestDistance = LLONG_MAX;

        // find the closest centroid, if the current data point index means it is a centroid then ?
        for (int c = 0; c < centroidIndices.size(); c++)
        {
                    cout << "XClusterKernel:y|1+" << c << endl;

            const int centroidLocalIndex = centroidIndices[c]; // local
                                cout << "XClusterKernel:y|2+" << c << endl;

            const int centroidGlobalIndex = dataIndices[centroidLocalIndex];
                                cout << "XClusterKernel:y|3+" << c << endl;

            auto currentCentroidData = data[centroidGlobalIndex];

                                cout << "XClusterKernel:y|4+" << c << endl;


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

        cout << "in cluster kernel 10" << nearestCentroidIndex << " Z " << i << endl;
        localThreadResults.push_back(make_pair(nearestCentroidIndex, i)); // dataIndex
    }
    return localThreadResults;
}

// centroidSeedIndices must be global
// cluster children are relative to the data indicies MUST BE TRANSLATED before prior step FIXME42
map<int, vector<int>> optimiseClusterMembership(vector<int> dataIndices, std::shared_ptr<FeatureMatrix> _data, vector<int> centroidSeedIndices, int processor_count)
{
    // auto dataIndices = *_dataIndices;
    auto data = *_data;
    map<int, bool> isCentroid = {};
    map<int, vector<int>> clusterMembership = {};

    const int dataRowCount = dataIndices.size();
    const int threadPool = processor_count;

    auto range = getRange(dataRowCount);
    auto distributedTasks = distributeTasks(range, threadPool);
    cout << "clusterx0" << endl;

    // for each centroid
    for (int c = 0; c < centroidSeedIndices.size(); c++)
    {
        cout << "clusterx1+c:" << c << endl;
        const int cCentroidIndex = centroidSeedIndices[c]; // this is local index in dataIndicies
                cout << "clusterx2+c:" << c << endl;

        isCentroid[cCentroidIndex] = true;
                cout << "clusterx1+3:" << c << endl;

        clusterMembership[cCentroidIndex] = {};
                cout << "clusterx1+4:" << c << endl;

    }

    vector<pair<int, int>> threadResults = {};
    vector<std::future<std::vector<std::pair<int, int>>>> futures = {};

    cout << "clusterx2" << endl;

    for (int i = 0; i < threadPool; i++)
    {
            cout << "clusterx3+i: " << i << endl;

        vector<int> threadTask = distributedTasks[i];
                    cout << "clusterx4+i: " << i << endl;

        auto indexClone = dataIndices;
                    cout << "clusterx5+i: " << i << endl;

        auto centroidSeedIndicesClone = centroidSeedIndices;
                    cout << "clusterx6+i: " << i << endl;

        auto future = std::async(std::launch::async, [_data, &isCentroid, threadTask = std::move(threadTask), indexClone = std::move(indexClone), centroidSeedIndices = std::move(centroidSeedIndicesClone)]()
                                 {
                                     return clusterKernel(indexClone, _data, threadTask, centroidSeedIndices, isCentroid);
                                 });
        futures.emplace_back(std::move(future));
                    cout << "clusterx7+i: " << i << endl;

    }

    for (int i = 0; i < futures.size(); i++)
    {
                        cout << "clustery5+:" << i << endl;

        auto data = futures[i].get();
                                cout << "clustery6+:" << i << endl;

        for (int i = 0; i < data.size(); i++)
        {
            clusterMembership[data[i].first].push_back(data[i].second);
        }
    }
    return clusterMembership;
}
