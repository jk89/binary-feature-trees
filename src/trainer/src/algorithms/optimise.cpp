#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <future>

using namespace std;

std::mutex optimiseSelectionCostMtx; // mutex for critical section

vector<tuple<int, int, long long, long long>> optimiseSelectionCostKernel(std::shared_ptr<FeatureMatrix> _data, vector<int> threadTasks, vector<vector<int>> &clusters, vector<tuple<int, int>> &tasks) // vector<tuple<int, int, long long, long long>> &resultSet
{
    auto data = *_data;
    map<int, bool> clusterExists = {};

    // map<clusterId> => bestCenteroidId, bestCentroidCost, totalCost
    map<int, tuple<int, long long, long long>> results = {};
    int taskMax = threadTasks.size();
    int j = 0;
    for (auto it = threadTasks.begin(); it != threadTasks.end(); it++)
    {
        auto task = tasks[*it];
        int clusterId = get<0>(task);
        int pointId = get<1>(task);

        vector<int> cluster(clusters[clusterId]); // get<2>(task);

        // get data index of point id
        const int pointGlobalIndex = cluster[pointId];

        // remove pointId from cluster
        cluster.erase(cluster.begin() + pointId);
        auto candidateData = data[pointGlobalIndex];
        long long cost = 0;

        for (int k = 0; k < cluster.size(); k++)
        {
            auto dataData = data[cluster[k]];
            int distance = hammingDistance(candidateData, dataData);
            cost += distance;
        }

        if (clusterExists[clusterId]) // results.find(clusterId) != results.end() // results.find(clusterId) != results.end() // clusterExists[clusterId]
        {
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
        }
        else
        {
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

pair<long long, map<int, vector<int>>> optimiseCentroidSelectionAndComputeClusterCost(std::shared_ptr<FeatureMatrix> _data, map<int, vector<int>> &clusterMembership, int processor_count)
{
    auto data = *_data;
    vector<int> centroids = getClusterKeys(clusterMembership);
    vector<vector<int>> clusters = {};

    vector<tuple<int, int>> tasks = {};
    for (int i = 0; i < centroids.size(); i++)
    {
        int centroid = centroids[i];
        vector<int> cluster(clusterMembership[centroid]);
        cluster.push_back(centroid);
        clusters.push_back(cluster);
        for (int j = 0; j < cluster.size(); j++)
        {
            tasks.push_back(make_tuple(i, j));
        }
    }

    auto distributedTasks = distributeTasks(tasks, processor_count);

    vector<std::future<std::vector<std::tuple<int, int, long long, long long>>>> futures = {};
    vector<tuple<int, int, long long, long long>> resultSet = {}; // clusterId, bestCentroidId, bestCentroidCost, totalCost

    int ix = 0;
    for (map<int, vector<int>>::iterator it = distributedTasks.begin(); it != distributedTasks.end(); ++it)
    {
        auto task = distributedTasks[it->first];
        auto future = std::async(std::launch::async, [&, task = std::move(task)]()
                                 { return optimiseSelectionCostKernel(_data, task, clusters, tasks); });
        futures.emplace_back(std::move(future));
        ix++;
    }

    map<int, bool> resultHasCluster = {};
    map<int, tuple<int, long long, long long>> resultSetAgg = {}; // clusterId => bestCentroidId, bestCentroidCost, totalCost

    for (int i = 0; i < futures.size(); i++)
    {
        std::vector<std::tuple<int, int, long long, long long>> data;
        data = futures[i].get();

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
                }

                resultSetAgg[clusterId] = make_tuple(bestGlobalCentroidIndex, bestGlobalCentroidCost, newGlobalTotal);
            }
            else
            {
                resultSetAgg[clusterId] = make_tuple(bestCentroidId, bestCentroidCost, totalCost);
                resultHasCluster[clusterId] = true;
            }
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
        const int bestCentroidGlobal = fullCluster[bestCentroid];
        // erase best centroid
        fullCluster.erase(fullCluster.begin() + bestCentroid);
        newClusterMembership[bestCentroidGlobal] = fullCluster;
    }

    return make_pair(totalCost, newClusterMembership);
}
