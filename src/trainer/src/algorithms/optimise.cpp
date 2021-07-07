#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <future>

using namespace std;

std::mutex optimiseSelectionCostMtx; // mutex for critical section

vector<tuple<int, int, long long, long long>> optimiseSelectionCostKernel(vector<int> dataIndices, std::shared_ptr<FeatureMatrix> _data, vector<int> threadTasks, vector<vector<int>> &clusters, vector<tuple<int, int>> &tasks) // vector<tuple<int, int, long long, long long>> &resultSet
{
    auto data = *_data;
    map<int, bool> clusterExists = {};

    // map<clusterId> => bestCenteroidId, bestCentroidCost, totalCost
    map<int, tuple<int, long long, long long>> results = {};
    int taskMax = threadTasks.size();
    int j = 0;
    for (auto it = threadTasks.begin(); it != threadTasks.end(); it++)
    {
        // cout << "1" << endl;
        // so task is a list of local centroid ids (i) and their cluster members (j)
        // where the centroid local id is also included in the cluster membership
        auto task = tasks[*it];
        int clusterId = get<0>(task); // centroidDataPoint local index of the centroid aka 1st, 2nd centroid
        // which can be found in
        int pointId = get<1>(task); // clusterDataPoint this is the idx within a particular clusters[clusterId] which includes the prior centroids index in this list

        vector<int> cluster(clusters[clusterId]); // get<2>(task);
        // vector<int> clusterOriginal(clusters[clusterId]);

        // get data index of point id (point is a candidate as a new centroid)
        const int pointLocalIndex = cluster[pointId];
        // cout << "2" << endl;
        const int pointGlobalIndex = dataIndices[pointLocalIndex];
        // cout << pointLocalIndex << endl;
        // cout << pointGlobalIndex << endl;
        // cout << "3" << endl;
        // remove pointId from cluster
        cluster.erase(cluster.begin() + pointId);
        //cout << "4" << endl;
        auto candidateData = data[pointGlobalIndex];
        // cout << "5" << endl;
        long long cost = 0;

        for (int k = 0; k < cluster.size(); k++) // for the remaining cluster points
        {
            // cout << "k0+ " << k << endl;
            auto localId = cluster[k];

            // cout << "k1+ " << k << endl;

            auto globalId = dataIndices[localId];
            // cout << "k2+ " << k << endl;

            auto dataData = data[globalId];
            // cout << "k3+ " << k << endl;

            int distance = hammingDistance(candidateData, dataData);
            // cout << "k4+ " << k << endl;

            cost += distance;
        }

        if (clusterExists[clusterId]) // results.find(clusterId) != results.end() // results.find(clusterId) != results.end() // clusterExists[clusterId]
        {
            // cout << "cluster exist" << clusterId << endl;
            // bestCenteroidId, bestCentroidCost, totalCost
            int bestCentroidCost = get<1>(results[clusterId]);
            int bestCentroidIndex = get<0>(results[clusterId]);
            long long newTotal = get<2>(results[clusterId]) + cost;

            if (cost < bestCentroidCost)
            {
                // cout << "cost imp" << endl;
                bestCentroidCost = cost;
                bestCentroidIndex = pointId;
                cout << "winning point" << pointId << endl;
            }

            results[clusterId] = make_tuple(bestCentroidIndex, bestCentroidCost, newTotal);
        }
        else
        {
            // cout << "cluster not exist" << endl;
            cout << "winning point" << pointId << endl;

            results[clusterId] = make_tuple(pointId, cost, cost);
            clusterExists[clusterId] = true;
        }
        j++;
    }

    vector<tuple<int, int, long long, long long>> localResultSet = {}; // clusterId, bestCentroidId, bestCentroidCost, totalCost

    for (map<int, tuple<int, long long, long long>>::iterator it = results.begin(); it != results.end(); it++)
    {
        auto key = it->first;
        auto value = results[it->first];
        localResultSet.push_back(make_tuple(key, get<0>(value), get<1>(value), get<2>(value)));
    }

    return localResultSet;
}

// FIXME NEEDS DATA INDICES due to FIXME42
tuple<long long, map<int, vector<int>>, vector<int>> optimiseCentroidSelectionAndComputeClusterCost(vector<int> dataIndices, std::shared_ptr<FeatureMatrix> _data, vector<int> centroids, map<int, vector<int>> &clusterMembership, int processor_count)
{
    auto data = *_data;
    // vector<int> centroids = getClusterKeys(clusterMembership); // local
    vector<vector<int>> clusters = {};

    vector<tuple<int, int>> tasks = {};
    for (int i = 0; i < centroids.size(); i++)
    {
        int centroid = centroids[i];                      // local to data indicies
        vector<int> cluster(clusterMembership[i]); // array of cluster but without the centroid // centroid
        cluster.push_back(centroid);                      // add the centroid
        clusters.push_back(cluster);                      // add the cluster
        cout << "in opt, cidx" << i << "|" << cluster.size() << endl;
        for (int j = 0; j < cluster.size(); j++)
        {
            tasks.push_back(make_tuple(i, j)); // so task is a list of local centroid ids (i) and their cluster members (j)
            // where the centroid local id is also included in the cluster membership
        }
    }

    auto distributedTasks = distributeTasks(tasks, processor_count);

    vector<std::future<std::vector<std::tuple<int, int, long long, long long>>>> futures = {};
    vector<tuple<int, int, long long, long long>> resultSet = {}; // clusterId, bestCentroidId, bestCentroidCost, totalCost

    int ix = 0;
    for (map<int, vector<int>>::iterator it = distributedTasks.begin(); it != distributedTasks.end(); ++it)
    {
        // cout << "idx in opt" << ix << endl;
        auto task = distributedTasks[it->first];
        auto future = std::async(std::launch::async, [&, task = std::move(task)]()
                                 { return optimiseSelectionCostKernel(dataIndices, _data, task, clusters, tasks); });
        futures.emplace_back(std::move(future));
        ix++;
    }

    map<int, bool> resultHasCluster = {};
    map<int, tuple<int, long long, long long>> resultSetAgg = {}; // clusterId => bestCentroidId, bestCentroidCost, totalCost

    for (int i = 0; i < futures.size(); i++)
    {
        // cout << "i in opt get" << i << endl;
        std::vector<std::tuple<int, int, long long, long long>> data;
        data = futures[i].get();
        // cout << "i in opt got" << i << endl;

        for (int i = 0; i < data.size(); i++)
        {
            auto clusterId = get<0>(data[i]);
            auto bestCentroidId = get<1>(data[i]);
            auto bestCentroidCost = get<2>(data[i]);
            auto totalCost = get<3>(data[i]);
            if (resultHasCluster[clusterId] == true) // resultSetAgg.count(clusterId) > 0
            {
                int bestGlobalCentroidCost = get<1>(resultSetAgg[clusterId]);
                int bestGlobalCentroidIndex = get<0>(resultSetAgg[clusterId]);
                long long newGlobalTotal = get<2>(resultSetAgg[clusterId]) + totalCost;

                if (bestCentroidCost < bestGlobalCentroidCost)
                {
                    bestGlobalCentroidCost = bestCentroidCost;
                    bestGlobalCentroidIndex = bestCentroidId;

                    cout << "agg 2" << bestCentroidId << endl;
                }

                resultSetAgg[clusterId] = make_tuple(bestGlobalCentroidIndex, bestGlobalCentroidCost, newGlobalTotal);
            }
            else
            {
                                    cout << "agg 1" << bestCentroidId << endl;

                resultSetAgg[clusterId] = make_tuple(bestCentroidId, bestCentroidCost, totalCost);
                resultHasCluster[clusterId] = true;
            }
        }
    }

    // contrust total cost for all clusters
    // get new map of centroids vs cluster
    map<int, vector<int>> newClusterMembership = {};
    vector<int> newCentroidIndices = {};
    long long totalCost = 0;
    for (int i = 0; i < centroids.size(); i++)
    {
        /*
        ok so each bestCentroid is the idx found within a cluster
        so cluster[i][bestCentroid] is the dataIndicies idx which needs to be scrubbed to form the new children
        
        all we need to do is remove the bestCentroidId from cluster[i]
        */
        auto clusterResults = resultSetAgg[i]; // make_tuple(bestGlobalCentroidIndex, bestGlobalCentroidCost, newGlobalTotal);
        totalCost += get<2>(clusterResults);
        int bestCentroid = get<0>(clusterResults); // this is local to dataIndicies
        
        // int bestLocalCentroidIdx = dataIndices[bestCentroid];
        // int oldCentroidId = centroids[i];
        auto fullCluster = clusters[i]; // cluster including old centroid and new centroid

        // const int bestCentroidGlobal = fullCluster[bestCentroid]; // get centroid
        // erase best centroid
        fullCluster.erase(fullCluster.begin() + bestCentroid);
        newClusterMembership[i] = fullCluster;
        newCentroidIndices.push_back(bestCentroid);
    }

    return make_tuple(totalCost, newClusterMembership, newCentroidIndices);
}
