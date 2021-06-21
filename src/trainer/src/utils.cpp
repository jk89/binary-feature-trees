#include <boost/filesystem.hpp>
#include <random>
#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
// #include "models.cpp"

using namespace boost::filesystem;
namespace fs = boost::filesystem;
using namespace std;


/*

#ifndef ConcurrentIndexRange
#include "models.cpp"
#define ConcurrentIndexRange
#endif

template <class myType>
myType getKey (myType a) {
 return (a>b?a:b);
}
*/

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

    return taskDistibution;
}


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
    int distance = cv::norm(v1, v2, cv::NORM_HAMMING);
    return distance;
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