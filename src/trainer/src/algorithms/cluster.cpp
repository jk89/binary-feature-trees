#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <thread>
#include <future>

using namespace std;

std::mutex clusterMtx; // mutex for critical section
//                                  { return clusterKernel(_dataIndices, _data, distributedTasks[i], centroidSeedIndices, isCentroid); });

vector<pair<int, int>> clusterKernel(vector<int> *_dataIndices, std::shared_ptr<FeatureMatrix> _data, vector<int> range, vector<int> centroidIndices, map<int, bool> isCentroid) // map<int, vector<int>> &clusterMembership  vector<pair<int, int>> &threadResults
{
    // cout << "here   " << endl;
    auto dataIndices = *_dataIndices;
    auto data = *_data;
    // auto range = *_range;

    cout << "inclusterkernel " << dataIndices.size() << ", " << data.size() << endl;
    cout << "range " << range.size() << endl;
    cout << "sending params3 " << endl;
    centroidPrinter(range);
    // cout << "ok" << endl;
    vector<pair<int, int>> localThreadResults;

    // iterate data within range
    for (int x = 0; x < range.size(); x++)
    {
        // cout << "here  2  " << endl;
        int i = range[x];
        // cout << "here  3 " << endl;
        bool isDataPointACentroid = isCentroid[i];
        // cout << "here 4" << endl;
        if (isDataPointACentroid == true)
            continue; // skip calculating optimal membership for centroids as they are not members of any cluster
        // cout << "here 5" << endl;

        // convert range index to data index
        int dataIndex = dataIndices[i];
        cout << "here 6z" << endl;
        // crashed here // what about using a shared pointer?
        auto currentFeatureData = data[(dataIndex)]; // row

        cout << "here 7" << endl;

        int nearestCentroidIndex = -1;
        long long nearestDistance = LLONG_MAX;

        // find the closest centroid, if the current data point index means it is a centroid then ?
        for (int c = 0; c < centroidIndices.size(); c++)
        {
            const int centroidIndex = centroidIndices[c];
            auto currentCentroidData = data[centroidIndex]; // row

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
            // exit(1);
        }

        // clusterMembership[nearestCentroidIndex].push_back(dataIndex);
        cout << "di " << dataIndex << "ci " << nearestCentroidIndex << endl;
        localThreadResults.push_back(make_pair(nearestCentroidIndex, dataIndex));
    }
    return localThreadResults;
    // clusterMtx
    /*clusterMtx.lock();
    threadResults.insert(threadResults.end(), localThreadResults.begin(), localThreadResults.end());
    clusterMtx.unlock();*/
}

//     auto distributedTasks = distributeTasks(tasks, processor_count);

map<int, vector<int>> optimiseClusterMembership(vector<int> *_dataIndices, std::shared_ptr<FeatureMatrix> _data, vector<int> &centroidSeedIndices, int processor_count)
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

    auto range = getRange(dataRowCount);
    auto distributedTasks = distributeTasksVec(range, threadPool);

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

    // vector<pair<int, int>>

    // vector<thread> threads(threadPool);
    vector<pair<int, int>> threadResults = {};
    // std::future<std::vector<std::pair<int, int>>> future
    vector<std::future<std::vector<std::pair<int, int>>>> futures = {};

    cout << " initing the pools " << endl;

    // FIX ME POSSIBLE BUG HERE

    for (int i = 0; i < threadPool; i++)
    {
        // void clusterKernel(vector<int> &dataIndices, cv::Mat &data, ConcurrentIndexRange &range, vector<int> &centroidIndices, map<int, bool> &isCentroid, map<int, vector<int>> &clusterMembership)
        // vector<CentroidDataIndexPair> &threadResults
        // ranges[i]
        /*

        */
        // const std::vector<int>& v2 = v1;
        vector<int> thisTasks = distributedTasks[i];

        cout << "sending params1 " << endl;
        centroidPrinter(thisTasks);

        auto future = std::async(std::launch::async, [&_dataIndices, &_data, &centroidSeedIndices, &isCentroid, thisTasks = std::move(thisTasks)]()
                                 {
                                     cout << "sending params2 " << endl;
                                     centroidPrinter(thisTasks);
                                     return clusterKernel(_dataIndices, _data, thisTasks, centroidSeedIndices, isCentroid);
                                 });
        futures.emplace_back(std::move(future));
        // futures.push_back(std::move(future));
        /*cout << "ranges i start" << ranges[i].start << endl;
        cout << "ranges i end" << ranges[i].end << endl;
        for (int c = 0; c < centroidSeedIndices.size(); c++)
        {
            cout << " centroid " << centroidSeedIndices[c] << endl;
        }*/

        // threads[i] = (thread{clusterKernel, _dataIndices, _data, ref(distributedTasks[i]), ref(centroidSeedIndices), ref(isCentroid), ref(threadResults)}); // thread(doSomething, i + 1);
        cout << "building thread " << i << " done" << endl;
    }

    cout << " about to join " << endl;

    /*for (int i = 0; i < futures.size(); i++)
    {
        futures[i].wait();
    }
*/
    for (int i = 0; i < futures.size(); i++)
    {
        cout << "0here 0" << endl;
        cout << futures[i].valid() << endl;
        // crashed here
        auto data = futures[i].get();
        cout << "0here 1" << endl;

        for (int i = 0; i < data.size(); i++)
        {
            cout << "0here 2+" << i << endl;
            // crashed here with i=0,1
            // cout << " pair results, data index: " << threadResults[i].second << " centroid index " << threadResults[i].first << endl;
            // what about using emplace with degenerate same key map entries
            // https://stackoverflow.com/questions/4303513/push-back-vs-emplace-back
            // what if first is -1 from above
            clusterMembership[data[i].first].push_back(data[i].second);
        }
        cout << "0here END OF FUTURE" << endl;

        // threadResults.insert(threadResults.end(), data.begin(), data.end());
    }

    /*int j = 0;
    for (auto &th : threads)
    {
        cout << "joining thread " << j << endl;
        th.join();
        j++;
    }*/

    // cout << " RESULT SET SIZE " << threadResults.size() << endl;

    // FIXME POSSIBLE BUG HERE
    /*for (int i = 0; i < threadResults.size(); i++)
    {
        // cout << " pair results, data index: " << threadResults[i].second << " centroid index " << threadResults[i].first << endl;
        clusterMembership[threadResults[i].first].push_back(threadResults[i].second);
    }*/
    // cout << endl;

    clusterMembershipPrinter(clusterMembership);

    cout << " about to return cluster membeship" << endl;
    // m = cv::Mat(dedupVectorData.size(), 32, CV_8U)
    // map<int, vector<int>> clusterMembership = {};
    return clusterMembership;
}
