#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
using namespace std;


std::mutex optimiseSelectionCostMtx; // mutex for critical section
// map<int, vector<int>> map<int, vector<int>>
//  ConcurrentIndexRange &range
void optimiseSelectionCostKernel(cv::Mat &data, vector<int> &threadTasks, vector<vector<int>> &clusters, vector<tuple<int, int>> &tasks, vector<tuple<int, int, long long, long long>> &resultSet)
{
    using std::chrono::duration;
    using std::chrono::duration_cast;
    using std::chrono::high_resolution_clock;
    using std::chrono::milliseconds;
    map<int, bool> clusterExists = {};

    // map<clusterId> => bestCenteroidId, bestCentroidCost, totalCost
    map<int, tuple<int, long long, long long>> results = {};
    int taskMax = threadTasks.size();
    int j = 0;
    for (auto it = threadTasks.begin(); it != threadTasks.end(); it++)
    {
        auto t1 = high_resolution_clock::now();
        auto task = tasks[*it];
        int clusterId = get<0>(task);
        int pointId = get<1>(task);

        vector<int> cluster = clusters[clusterId]; // get<2>(task);

        // get data index of point id
        const int pointGlobalIndex = cluster[pointId];

        // remove pointId from cluster
        cluster.erase(cluster.begin() + pointId);

        auto candidateData = data.row(pointGlobalIndex);

        // long long sumCost = 0;
        // long long bestCost = long long_MAX;
        // int bestCentroidIndex = - 1;

        long long cost = 0;
        for (int k = 0; k < cluster.size(); k++)
        {
            auto dataData = data.row(cluster[k]);
            int distance = hammingDistance(candidateData, dataData);
            cost += distance;
        }

        if (clusterExists[clusterId]) // results.find(clusterId) != results.end() // results.find(clusterId) != results.end() // clusterExists[clusterId]
        {
            // exists
            // bestCenteroidId, bestCentroidCost, totalCost
            int bestCentroidCost = get<1>(results[clusterId]);
            int bestCentroidIndex = get<0>(results[clusterId]);
            long long newTotal = get<2>(results[clusterId]) + cost;

            if (cost < bestCentroidCost)
            {
                bestCentroidCost = cost;
                bestCentroidIndex = pointId;
            }
            results[clusterId] = make_tuple(bestCentroidIndex, bestCentroidCost, newTotal);

            // auto existingResults = results[clusterId];
        }
        else
        {
            results[clusterId] = make_tuple(pointId, cost, cost);
            clusterExists[clusterId] = true;
        }

        if (j % 97 == 0)
        {
            auto t2 = high_resolution_clock::now();
            auto ms_int = duration_cast<milliseconds>(t2 - t1);
            cout << "thread " << this_thread::get_id() << " | local idx" << j << " | global idx " << *it << " | pc " << ((float(j)) * 100) / (taskMax) << endl;
            std::cout << ms_int.count() << "ms\n";
        }

        // cout << " done task " << i << endl;
        j++;
        // mymap.count(c)>0
    }

    vector<tuple<int, int, long long, long long>> localResultSet = {}; // clusterId, bestCentroidId, bestCentroidCost, totalCost

    for (map<int, tuple<int, long long, long long>>::iterator it = results.begin(); it != results.end(); ++it)
    {
        // keys.push_back(it->first);
        auto key = it->first;
        auto value = it->second;

        localResultSet.push_back(make_tuple(key, get<0>(value), get<1>(value), get<2>(value)));
    }

    //
    optimiseSelectionCostMtx.lock();
    resultSet.insert(resultSet.end(), localResultSet.begin(), localResultSet.end());
    optimiseSelectionCostMtx.unlock();
}


pair<long long, map<int, vector<int>>> optimiseCentroidSelectionAndComputeCost(cv::Mat &data, map<int, vector<int>> &clusterMembership, int processor_count)
{
    vector<int> centroids = getClusterKeys(clusterMembership);
    vector<vector<int>> clusters = {};

    // jobs = [];
    // <centroidIdentifier{0,1,2,4},dataIdentifier{0,1,....1000000}
    vector<tuple<int, int>> tasks = {};
    for (int i = 0; i < centroids.size(); i++)
    {
        int centroid = centroids[i];
        vector<int> cluster = clusterMembership[centroid];
        cluster.push_back(centroid);
        clusters.push_back(cluster);
        // sort(cluster.begin(), cluster.end());
        for (int j = 0; j < cluster.size(); j++)
        {
            // TODO FIXE ME, rather than pushing the cluster, push the cluster index and parse the reference to the kernel
            tasks.push_back(make_tuple(i, j)); // j is the data point index
            // for each task we need to know:
            // cluster id, data point id, cluster indicies pointer
        }
    }

    auto distributedTasks = distributeTasks(tasks, processor_count);

    vector<thread> threads(processor_count);
    vector<tuple<int, int, long long, long long>> resultSet = {}; // clusterId, bestCentroidId, bestCentroidCost, totalCost

    cout << " about to optimisse selection. tasks:" << tasks.size() << endl;

    int ix = 0;
    for (map<int, vector<int>>::iterator it = distributedTasks.begin(); it != distributedTasks.end(); ++it)
    {
        threads[ix] = thread{optimiseSelectionCostKernel, ref(data), ref(distributedTasks[it->first]), ref(clusters), ref(tasks), ref(resultSet)};
        ix++;
    }
    /*for (int i = 0; i < ranges.size(); i++)
    {
        // void optimiseSelectionCostKernel(cv::Mat &data, ConcurrentIndexRange range, vector<tuple<int, int, vector<int>>> tasks, vector<tuple<int, int, long long, long long>> &resultSet)

        threads[i] = thread{optimiseSelectionCostKernel, ref(data), ref(ranges[i]), ref(clusters), ref(tasks), ref(resultSet)};
    }*/
    for (auto &th : threads)
    {
        th.join();
    }

    map<int, bool> resultHasCluster = {};
    map<int, tuple<int, long long, long long>> resultSetAgg = {}; // clusterId => bestCentroidId, bestCentroidCost, totalCost
    for (int i = 0; i < resultSet.size(); i++)
    {
        auto clusterId = get<0>(resultSet[i]);
        auto bestCentroidId = get<1>(resultSet[i]);
        auto bestCentroidCost = get<2>(resultSet[i]);
        auto totalCost = get<3>(resultSet[i]);
        if (resultHasCluster[clusterId] == true) // resultSetAgg.count(clusterId) > 0
        {
            int bestGlobalCentroidCost = get<1>(resultSetAgg[clusterId]);
            int bestGlobalCentroidIndex = get<0>(resultSetAgg[clusterId]);
            long long newGlobalTotal = get<2>(resultSetAgg[clusterId]) + totalCost;

            if (bestCentroidCost < bestGlobalCentroidCost)
            {
                bestGlobalCentroidCost = bestCentroidCost;
                bestGlobalCentroidIndex = bestCentroidId;
            }
            resultSetAgg[clusterId] = make_tuple(bestGlobalCentroidIndex, bestGlobalCentroidCost, newGlobalTotal);
        }
        else
        {
            resultSetAgg[clusterId] = make_tuple(bestCentroidId, bestCentroidCost, totalCost);
            resultHasCluster[clusterId] = true;
        }
    }

    // contrust total cost for all clusters
    // get new map of centroids vs cluster
    map<int, vector<int>> newClusterMembership = {};
    long long totalCost = 0;
    for (int i = 0; i < centroids.size(); i++)
    {
        auto clusterResults = resultSetAgg[i];
        totalCost += get<2>(clusterResults);
        int bestCentroid = get<0>(clusterResults);
        // int oldCentroidId = centroids[i];
        auto fullCluster = clusters[i];
        // erase best centroid
        fullCluster.erase(fullCluster.begin() + bestCentroid);
        newClusterMembership[bestCentroid] = fullCluster;
    }

    return make_pair(totalCost, newClusterMembership);

    // so for results we need

    //         if (results.count(clusterId) > 0)
}
