#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>
#include <stdlib.h> /* atoi */
#include <thread>
#include <random>
#include <iterator>
#include <algorithm>
#include <chrono>

// using namespace cv;
using namespace std;
using namespace boost::filesystem;
using namespace std::chrono;

namespace fs = boost::filesystem;

const char *filename = "./data/ORBvoc.txt";
const auto processor_count = std::thread::hardware_concurrency();

struct ConcurrentIndexRange
{
    int start;
    int end;
};

struct CentroidDataIndexPair
{
    int centroidIndex;
    int dataGlobalIndex;
};
struct compareFeatureVecs
{
    bool operator()(const array<uint8_t, 32> &a, const array<uint8_t, 32> &b) const
    {
        array<uint8_t, 32> distance = {};
        for (int i = 0; i < 32; ++i)
        {
            distance[i] = a[i] - b[i];
            distance[i] = b[i] - a[i];
        }
        int total = 0;
        for (int i = 0; i < 32; ++i)
        {
            total += distance[i];
        }
        return total > 0;
    }
};

/**
 * \brief   Return the filenames of all files that have the specified extension
 *          in the specified directory and all subdirectories.
 */
std::vector<fs::path> get_all(fs::path const &root) // , std::string const & ext
{
    std::vector<fs::path> paths;
    if (fs::exists(root) && fs::is_directory(root))
    {
        for (auto const &entry : fs::directory_iterator(root)) // recursive_directory_iterator
        {
            if (fs::is_regular_file(entry)) //  && entry.path().extension() == ext
            {
                cout << entry.path().filename() << endl;
                paths.emplace_back(entry.path().filename());
            };
        }
    }

    return paths;
}

vector<array<uint8_t, 32>> sample(vector<array<uint8_t, 32>> input, int size)
{
    size_t maxSize = input.size();
    vector<array<uint8_t, 32>> output(size);
    for (int i = 0; i < size; i++)
    {
        int idx = (rand() % maxSize);
        output[i] = input[idx];
    }
    return output;
}

cv::Mat load_data()
{
    cv::Mat m;
    std::ifstream file(filename);
    // file.open(filename, ios::in|ios::out|ios::binary);
    get_all("./data");
    if (file.is_open())
    {
        std::string line;
        int idx = 0;
        vector<array<uint8_t, 32>> data = {};
        // for each line in file
        while (std::getline(file, line))
        {
            if (idx < 1)
            {
                idx++;
                continue;
            };
            std::string cline;
            cline = line.c_str();
            vector<std::string> strs;
            boost::split(strs, line, boost::is_any_of(" ")); // space delimeter
            strs.erase(strs.begin());
            // strip out non-orb data leaving just the orb features
            strs.erase(strs.begin());
            strs.erase(strs.begin() + 34);
            strs.erase(strs.begin() + 34);
            // decode feature
            array<uint8_t, 32> feature = {};
            int featureIdx = 0;
            for (auto it = strs.begin(); it != strs.end(); ++it)
            {
                std::string cfeature = *it;
                int icfeature = std::stoi(cfeature);
                uint8_t ui8feature = icfeature;
                feature[featureIdx] = ui8feature;
                featureIdx++;
            }
            // push feature to data vec
            data.push_back(feature);
            idx++;
        }
        file.close();
        cout << " Loaded file into vector " << endl;

        // create set for deduplication
        std::set<array<uint8_t, 32>, compareFeatureVecs> dedupedSetData;
        for (int i = 0; i < data.size(); ++i)
        {
            dedupedSetData.insert(data[i]);
        }
        cout << " Deduplicated features " << endl;

        // convert back to vector now its all unique
        vector<array<uint8_t, 32>> dedupVectorData = {}; // uint8_t[32]
        dedupVectorData.assign(dedupedSetData.begin(), dedupedSetData.end());

        vector<array<uint8_t, 32>> dedupVectorDataSubSample = dedupVectorData; // sample(dedupVectorData, 20000); // 

        cout << " Converted unique feature set into vector " << endl;

        m = cv::Mat(dedupVectorDataSubSample.size(), 32, CV_8U); // row col
        memcpy(m.data, dedupVectorDataSubSample.data(), dedupVectorDataSubSample.size() * sizeof(array<uint8_t, 32>));
        cout << " Loaded vector into matrix " << endl;
    }
    return m;
}

int random(int min, int max) //range : [min, max]
{
    static bool first = true;
    if (first)
    {
        srand(time(NULL)); //seeding for the first time only!
        first = false;
    }
    return min + rand() % ((max + 1) - min);
}

int hammingDistance(cv::Mat v1, cv::Mat v2)
{
    /*cout << "v1 " << cv::format(v1, cv::Formatter::FMT_PYTHON) << endl;
    cout << "v1 rows " << v1.rows << endl;
    cout << "v1 cols " << v1.cols << endl;

    cout << "v2 " << cv::format(v2, cv::Formatter::FMT_PYTHON) << endl;
    cout << "v2 rows " << v2.rows << endl;
    cout << "v2 cols " << v2.cols << endl;*/

    // result += popCountTable[a[i] ^ b2[i]];
    // bitwise_xor(drawing1,drawing2,res);
    // float * testData1D = (float*)testDataMat.data;
    // cv::Mat distance;
    // cv::bitwise_xor(v1, v2, distance);
    int distance = cv::norm(v1, v2, cv::NORM_HAMMING);

    return distance;

    // cout << " dist " << distance << endl;
    // return 0;
}

int seedKernel(cv::Mat data, int currentCentroidIndex, vector<int> centroids, int k) // data, dataIdValue, centroids, k, metric
{
    cout << "seed kernel " << currentCentroidIndex << endl;
    int maxDistance = 0;
    int maxIndex = -1;
    // for the whole dataset of features
    for (int r = 0; r < data.rows; r++)
    {
        // cout << "  r " << r << endl;
        // extract feature from row index
        cv::Mat currentDatasetFeature = data.row(r);
        int minDistance = INT_MAX;
        // for each already selected centroid
        for (int c = 0; c < centroids.size(); c++)
        {
            // cout << " c " << c << endl;
            int currentCentroidDataSetIndex = centroids[c];
            cv::Mat currentCentroidFeature = data.row(currentCentroidDataSetIndex);
            // currentDatasetFeature
            const int distance = hammingDistance(currentDatasetFeature, currentCentroidFeature);
            // min minDistance
            minDistance = min(distance, minDistance);
        }
        if (minDistance > maxDistance)
        {
            maxDistance = minDistance;
            maxIndex = r;
        }
    }
    return maxIndex; // return the best centroid
}

vector<int> seedClusters(cv::Mat data, int k) // data, k, metric
{
    // largest index
    const int dataLength = data.rows;
    vector<int> centroids = {};

    // add random first centroid index
    centroids.push_back(random(0, dataLength - 1));

    // for the rest of k
    for (int i = 0; i < k - 1; i++)
    {
        const int nextCentroid = seedKernel(data, i, centroids, k);
        centroids.push_back(nextCentroid);
    }

    return centroids;
}

// (clusterKernel) cv::Mat data, cv::Mat currentCentroidData, ConcurrentIndexRange range, vector<int> centroidIndices, cv::Mat distances

// dep'd as arg cv::Mat currentCentroidData,
std::mutex clusterMtx; // mutex for critical section

void clusterKernel(vector<int> &dataIndices, cv::Mat &data, ConcurrentIndexRange &range, vector<int> &centroidIndices, map<int, bool> &isCentroid, vector<pair<int, int>> &threadResults) // map<int, vector<int>> &clusterMembership
{
    // cout << "ok" << endl;
    vector<pair<int, int>> localThreadResults;

    // iterate data within range
    for (int i = range.start; i <= range.end; i++)
    {
        const bool isDataPointACentroid = isCentroid[i];
        if (isDataPointACentroid == true)
            continue; // skip calculating optimal membership for centroids as they are not members of any cluster

        // convert range index to data index
        int dataIndex = dataIndices[i];
        const cv::Mat currentFeatureData = data.row(dataIndex);

        int nearestCentroidIndex = -1;
        int nearestDistance = INT_MAX;

        // find the closest centroid, if the current data point index means it is a centroid then ?
        for (int c = 0; c < centroidIndices.size(); c++)
        {
            const int centroidIndex = centroidIndices[c];
            cv::Mat currentCentroidData = data.row(centroidIndex);

            // calculate pairwise hamming distance
            const int distance = hammingDistance(currentCentroidData, currentFeatureData);

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

vector<ConcurrentIndexRange> rangeCalculator(int count, int partitions)
{
    const int batchSize = (int)count / partitions;
    const int globalRemainder = count - (batchSize * partitions);

    vector<ConcurrentIndexRange> ranges = {};

    int countPartDiff = count - partitions;
    if (countPartDiff < 0)
    {
        partitions += countPartDiff;
    }

    // so what are the ranges
    for (int i = 0; i < partitions; i++)
    {
        int rangeStart = i * batchSize;
        int rangeEnd = ((i + 1) * batchSize) - 1;
        if (i == partitions - 1)
        {
            // isLast = true;
            rangeEnd = rangeEnd + globalRemainder;
        }
        cout << "start " << rangeStart << " end " << rangeEnd << endl;
        ConcurrentIndexRange range = {rangeStart, rangeEnd};
        ranges.push_back(range);
    }

    return ranges;
}

map<int, vector<int>> optimiseClusterMembership(vector<int> &dataIndices, cv::Mat &data, vector<int> &centroidSeedIndices)
{ // data, n=4, metric=hammingVector, intitalClusterIndices=None
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
    vector<pair<int, int>> threadResults;

    cout << " initing the pools " << endl;

    // FIX ME POSSIBLE BUG HERE

    for (int i = 0; i < threadPool; i++)
    {
        // void clusterKernel(vector<int> &dataIndices, cv::Mat &data, ConcurrentIndexRange &range, vector<int> &centroidIndices, map<int, bool> &isCentroid, map<int, vector<int>> &clusterMembership)
        // vector<CentroidDataIndexPair> &threadResults
        //
        threads[i] = (thread{clusterKernel, ref(dataIndices), ref(data), ref(ranges[i]), ref(centroidSeedIndices), ref(isCentroid), ref(threadResults)}); // thread(doSomething, i + 1);
    }

    for (auto &th : threads)
    {
        th.join();
    }

    cout << " RESULT SET SIZE " << threadResults.size() << endl;

    // FIXME POSSIBLE BUG HERE
    for (int i = 0; i < threadResults.size(); i++)
    {
        // cout << " pair results, data index: " << threadResults[i].second << " centroid index " << threadResults[i].first << endl;
        clusterMembership[threadResults[i].first].push_back(threadResults[i].second);
    }
    cout << endl;
    // m = cv::Mat(dedupVectorData.size(), 32, CV_8U)
    // map<int, vector<int>> clusterMembership = {};
    return clusterMembership;
}

/*
template <class myType>
myType getKey (myType a) {
 return (a>b?a:b);
}
*/

vector<int> getClusterKeys(map<int, vector<int>> m)
{
    //     map<int, vector<int>> clusterMembership = {};
    std::vector<int> keys;
    for (std::map<int, vector<int>>::iterator it = m.begin(); it != m.end(); ++it)
    {
        keys.push_back(it->first);
    }
    return keys;
}

// Surt's code, doesn't preserve sorted order
void erase_v4(std::vector<int> &vec, int value)
{
    // get the range in 2*log2(N), N=vec.size()
    auto bounds = std::equal_range(vec.begin(), vec.end(), value);

    // calculate the index of the first to be deleted O(1)
    auto last = vec.end() - std::distance(bounds.first, bounds.second);

    // swap the 2 ranges O(equals) , equal = std::distance(bounds.first, bounds.last)
    std::swap_ranges(bounds.first, bounds.second, last);

    // erase the victims O(equals)
    vec.erase(last, vec.end());
}

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

map<int, vector<int>> distributeTasks(vector<tuple<int, int>> &tasks, int partitions)
{
    map<int, vector<int>> taskDistibution = {};
    map<int, bool> partitionExists = {};
    int currentPartition = 0;
    for (int i = 0; i < tasks.size(); i++)
    {
        if (partitionExists[currentPartition] == false)
        {
            // make partition
            taskDistibution[currentPartition] = {};
            partitionExists[currentPartition] = true;
        }

        taskDistibution[currentPartition].push_back(i);

        currentPartition++;
        if (currentPartition % partitions == 0)
        {
            currentPartition = 0;
        }
    }

    /*for (map<int, vector<int>>::iterator it = taskDistibution.begin(); it != taskDistibution.end(); ++it)
    {
        cout << " thread " << it->first << endl;
        for (int i = 0; i < it->second.size(); i++)
        {
            cout << it->second[i] << ", ";
        }
        cout << endl;
        cout << endl;
        cout << endl;
    }*/

    return taskDistibution;
}

pair<long long, map<int, vector<int>>> optimiseCentroidSelectionAndComputeCost(cv::Mat &data, map<int, vector<int>> &clusterMembership)
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

void clusterMembershipPrinter(map<int, vector<int>> clusterMembership)
{
    cout << endl;
    cout << endl;
    cout << endl;
    cout << "--------------------------------------------------------------" << endl;
    for (map<int, vector<int>>::iterator centroid = clusterMembership.begin(); centroid != clusterMembership.end(); ++centroid)
    {
        cout << "CentroidIdx: " << centroid->first << endl;
        auto clusterMembers = clusterMembership[centroid->first];
        for (auto member = clusterMembers.begin(); member != clusterMembers.end(); ++member)
        {
            cout << *member << ", ";
        }
        cout << endl;
    }
    cout << "--------------------------------------------------------------" << endl;
    cout << endl;
    cout << endl;
    cout << endl;
}

int main(int argc, char **argv)
{
    cv::Mat m;
    m = load_data();
    cout << "details of m rows" << m.rows << " cols " << m.cols << " tyoe " << m.type() << endl;
    cout << "first row: " << cv::format(m.row(0), cv::Formatter::FMT_PYTHON) << endl;
    cout << "second row: " << cv::format(m.row(1), cv::Formatter::FMT_PYTHON) << endl;
    cout << "Seeding clusters:" << endl;
    vector<int> centroids = seedClusters(m, 8);
    cout << "Centroids: " << endl;
    for (auto i = centroids.begin(); i != centroids.end(); ++i)
    {
        std::cout << *i << ", ";
    }
    cout << endl;

    vector<int> indices;
    for (int i = 0; i < m.rows; i++)
    {
        indices.push_back(i);
    }

    // begin fitting
    long long bestCost = LLONG_MAX;
    auto bestMembership = optimiseClusterMembership(indices, m, centroids);
    bool escape = false;

    int iteration = 0;
    while (escape == false)
    {
        auto optimalSelectionResults = optimiseCentroidSelectionAndComputeCost(m, bestMembership);
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
        clusterMembership = optimiseClusterMembership(indices, m, centroids);
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

    cout << "ALL DONE. Best membership:" << endl;

    clusterMembershipPrinter(bestMembership);

    /*cout << "optimise------------------------> " << endl;
    map<int, vector<int>> clusterMembership = optimiseClusterMembership(indices, m, centroids);
    cout << "optimise resultsss:" << endl;

    clusterMembershipPrinter(clusterMembership);
    cout << "selllect----------------------->" << endl;
    auto optResults = optimiseCentroidSelectionAndComputeCost(m, clusterMembership);
    auto cost = optResults.first;
    clusterMembership = optResults.second;

    cout << "select resultsss:" << endl;
    clusterMembershipPrinter(clusterMembership);
    cout << "Cost: " << cost << endl;*/
}
