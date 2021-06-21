#include <boost/filesystem.hpp>
#include <random>
#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
// #include "models.cpp"

using namespace boost::filesystem;
namespace fs = boost::filesystem;
using namespace std;


/*

#define ConcurrentIndexRange

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



cv::Mat load_data(char *filename)
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

        vector<array<uint8_t, 32>> dedupVectorDataSubSample = sample(dedupVectorData, 10000); //  dedupVectorData; // 

        cout << " Converted unique feature set into vector " << endl;

        m = cv::Mat(dedupVectorDataSubSample.size(), 32, CV_8U); // row col
        memcpy(m.data, dedupVectorDataSubSample.data(), dedupVectorDataSubSample.size() * sizeof(array<uint8_t, 32>));
        cout << " Loaded vector into matrix " << endl;
    }
    return m;
}
