#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>
#include <stdlib.h> /* atoi */
#include <thread>

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

struct CentroidDataIndexPair {
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
        cout << " Converted unique feature set into vector " << endl;

        m = cv::Mat(dedupVectorData.size(), 32, CV_8U); // row col
        memcpy(m.data, dedupVectorData.data(), dedupVectorData.size() * sizeof(array<uint8_t, 32>));
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
std::mutex clusterMtx;           // mutex for critical section

void clusterKernel(vector<int> &dataIndices, cv::Mat &data, ConcurrentIndexRange &range, vector<int> &centroidIndices, map<int, bool> &isCentroid, vector<CentroidDataIndexPair> &threadResults) // map<int, vector<int>> &clusterMembership
{
    /*vector<CentroidDataIndexPair> localThreadResults;

    // iterate data within range
    for (int i = range.start; i <= range.end; i++)
    {
        const bool isDataPointACentroid = isCentroid[i];
        if (isDataPointACentroid == true) continue; // skip calculating optimal membership for centroids as they are not members of any cluster
        
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

            if (distance < nearestDistance) {
                nearestCentroidIndex = centroidIndex;
                nearestDistance = distance;
            }
        }

        // clusterMembership[nearestCentroidIndex].push_back(dataIndex);
        CentroidDataIndexPair indexPair = {nearestCentroidIndex, dataIndex};
        localThreadResults.push_back(indexPair);
    }*/
    // clusterMtx
    // clusterMtx.lock();
    // threadResults.insert(localThreadResults.end(), localThreadResults.begin(), localThreadResults.end());
    // clusterMtx.unlock();
}

void optimiseClusterMembership(vector<int> &dataIndices, cv::Mat &data, int k, vector<int> &centroidSeedIndices)
{ // data, n=4, metric=hammingVector, intitalClusterIndices=None
    // centroidData
    // processor_count
    // create centroidIndexMap
    map<int, bool> isCentroid = {};
    map<int, vector<int>> clusterMembership = {};

    const int dataRowCount = dataIndices.size(); // data.rows;
    const int threadPool = processor_count;
    const int batchSize = (int)dataRowCount / threadPool;
    // int lastBatchSize = batchSize;
    const int globalRemainder = dataRowCount - (batchSize * threadPool);
    // lastBatchSize = lastBatchSize + globalRemainder;
    cout << " thread pool " << threadPool << endl;
    cout << " batch size " << batchSize << endl;
    // cout << " last batch " << lastBatchSize << endl;
    cout << " total " << dataRowCount << endl;

    vector<ConcurrentIndexRange> ranges = {};

    // so what are the ranges
    for (int i = 0; i < threadPool; i++)
    {
        int rangeStart = i * batchSize;
        int rangeEnd = ((i + 1) * batchSize) - 1;
        if (i == threadPool - 1)
        {
            // isLast = true;
            rangeEnd = rangeEnd + globalRemainder;
        }
        cout << "start " << rangeStart << " end " << rangeEnd << endl;
        ConcurrentIndexRange range = {rangeStart, rangeEnd};
        ranges.push_back(range);
    }

    // centroidMembership

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
    vector<CentroidDataIndexPair> threadResults;

    for (int i = 0; i < threadPool; i++)
    {
        // void clusterKernel(vector<int> &dataIndices, cv::Mat &data, ConcurrentIndexRange &range, vector<int> &centroidIndices, map<int, bool> &isCentroid, map<int, vector<int>> &clusterMembership)
        // vector<CentroidDataIndexPair> &threadResults
        threads[i] = thread(clusterKernel, ref(dataIndices), ref(data), ref(ranges[i]), ref(centroidSeedIndices), ref(isCentroid), ref(threadResults)); // thread(doSomething, i + 1);
    }

    for (auto &th : threads)
    {
        th.join();
    }

    // m = cv::Mat(dedupVectorData.size(), 32, CV_8U)
}

int main(int argc, char **argv)
{
    cv::Mat m;
    m = load_data();
    cout << "details of m rows" << m.rows << " cols " << m.cols << " tyoe " << m.type() << endl;
    cout << "first row: " << cv::format(m.row(0), cv::Formatter::FMT_PYTHON) << endl;
    cout << "second row: " << cv::format(m.row(1), cv::Formatter::FMT_PYTHON) << endl;
    vector<int> centroids = seedClusters(m, 8);
    for (auto i = centroids.begin(); i != centroids.end(); ++i)
        std::cout << *i << ' ';
    
    vector<int> indices;
    for (int i = 0; i < m.rows; i++) {
        indices[i] = i;
    }

    optimiseClusterMembership(indices, m, 8, centroids);
}
