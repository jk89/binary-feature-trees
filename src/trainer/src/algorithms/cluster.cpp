#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <thread>

using namespace std;

std::mutex clusterMtx; // mutex for critical section

void clusterKernel(vector<int> *_dataIndices, cv::Mat *_data, ConcurrentIndexRange &range, vector<int> &centroidIndices, map<int, bool> &isCentroid, vector<pair<int, int>> &threadResults) // map<int, vector<int>> &clusterMembership
{   
    auto dataIndices = *_dataIndices;
    auto data = *_data;
    // cout << "ok" << endl;
    vector<pair<int, int>> localThreadResults;


    // iterate data within range
    for (int i = range.start; i <= range.end; i++)
    {
        bool isDataPointACentroid = isCentroid[i];
        if (isDataPointACentroid == true)
            continue; // skip calculating optimal membership for centroids as they are not members of any cluster

        // convert range index to data index
        int dataIndex = dataIndices[i];
        const cv::Mat currentFeatureData = data.row(dataIndex);

        int nearestCentroidIndex = -1;
        long long nearestDistance = LLONG_MAX;

        // find the closest centroid, if the current data point index means it is a centroid then ?
        for (int c = 0; c < centroidIndices.size(); c++)
        {
            const int centroidIndex = centroidIndices[c];
            cv::Mat currentCentroidData = data.row(centroidIndex);

            // calculate pairwise hamming distance
            long long distance = hammingDistance(currentCentroidData, currentFeatureData);

            if (distance < nearestDistance)
            {
                nearestCentroidIndex = centroidIndex;
                nearestDistance = distance;
            }
        }

        // clusterMembership[nearestCentroidIndex].push_back(dataIndex);
        // cout << "di " << dataIndex << "ci " << nearestCentroidIndex << endl;
        localThreadResults.push_back(make_pair(nearestCentroidIndex, dataIndex));
    }
    // clusterMtx
    clusterMtx.lock();
    threadResults.insert(threadResults.end(), localThreadResults.begin(), localThreadResults.end());
    clusterMtx.unlock();
}

map<int, vector<int>> optimiseClusterMembership(vector<int> *_dataIndices, cv::Mat *_data, vector<int> &centroidSeedIndices, int processor_count)
{ // data, n=4, metric=hammingVector, intitalClusterIndices=None
    cout << "ROUTINE: cluster" << endl; 
    auto dataIndices = *_dataIndices;
    auto data = *_data;
    // centroidData
    // processor_count
    // create centroidIndexMap
    map<int, bool> isCentroid = {};
    map<int, vector<int>> clusterMembership = {};

    const int dataRowCount = dataIndices.size(); // data.rows;
    const int threadPool = processor_count;

    vector<ConcurrentIndexRange> ranges = rangeCalculator(dataRowCount, threadPool);

    // centroidMembership
    cout << " about to init clusterMember ship is centroid store" << endl;

    // for each centroid
    for (int c = 0; c < centroidSeedIndices.size(); c++)
    {
        const int cCentroidIndex = centroidSeedIndices[c];
        isCentroid[cCentroidIndex] = true;
        clusterMembership[cCentroidIndex] = {};
    }

    //create empty distances matrix
    // cv::Mat distances = cv::Mat(data.size(), 1, CV_8U);

    // perform the distance between each data feature and the current centroid
    // const int centroidIndex = centroidSeedIndices[0];
    // const cv::Mat centroidData = data.row(centroidIndex);

    vector<thread> threads(threadPool);
    vector<pair<int, int>> threadResults = {};

    cout << " initing the pools " << endl;

    // FIX ME POSSIBLE BUG HERE

    for (int i = 0; i < threadPool; i++)
    {
        // void clusterKernel(vector<int> &dataIndices, cv::Mat &data, ConcurrentIndexRange &range, vector<int> &centroidIndices, map<int, bool> &isCentroid, map<int, vector<int>> &clusterMembership)
        // vector<CentroidDataIndexPair> &threadResults
        // ranges[i]
        /*

        */
        cout << "ranges i start" << ranges[i].start << endl;
        cout << "ranges i end" << ranges[i].end << endl;
        for (int c = 0; c < centroidSeedIndices.size(); c++)
        {
            cout << " centroid " << centroidSeedIndices[c] << endl;
        }

        threads[i] = (thread{clusterKernel, _dataIndices, _data, ref(ranges[i]), ref(centroidSeedIndices), ref(isCentroid), ref(threadResults)}); // thread(doSomething, i + 1);
        cout << "building thread " << i << " done" << endl;
    }

    cout << " about to join " << endl;

    int j = 0;
    for (auto &th : threads)
    {
        cout << "joining thread " << j << endl;
        th.join();
        j++;
    }

    cout << " RESULT SET SIZE " << threadResults.size() << endl;

    // FIXME POSSIBLE BUG HERE
    for (int i = 0; i < threadResults.size(); i++)
    {
        // cout << " pair results, data index: " << threadResults[i].second << " centroid index " << threadResults[i].first << endl;
        clusterMembership[threadResults[i].first].push_back(threadResults[i].second);
    }
    cout << endl;

    clusterMembershipPrinter(clusterMembership);
    // m = cv::Mat(dedupVectorData.size(), 32, CV_8U)
    // map<int, vector<int>> clusterMembership = {};
    return clusterMembership;
}
