#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <thread>
#include <future>

using namespace std;

vector<pair<int, int>> kernelTester(vector<int> range) // map<int, vector<int>> &clusterMembership  vector<pair<int, int>> &threadResults
{
    cout << "range " << range.size() << endl;
    cout << "sending params3 " << endl;
    centroidPrinter(range);
    return {};
}

//     auto distributedTasks = distributeTasks(tasks, processor_count);

void concurrentTester(vector<int> *_dataIndices, int processor_count)
{
    auto dataIndices = *_dataIndices;
    const int dataRowCount = dataIndices.size(); // data.rows;
    const int threadPool = processor_count;

    auto range = getRange(dataRowCount);
    auto distributedTasks = distributeTasksVec(range, threadPool);

    vector<std::future<std::vector<std::pair<int, int>>>> futures = {};
    for (int i = 0; i < threadPool; i++)
    {
        vector<int> thisTasks = distributedTasks[i];

        cout << "params1 " << endl;
        centroidPrinter(thisTasks);

        auto future = std::async(std::launch::async, [thisTasks = std::move(thisTasks)]()
                                 {
                                     cout << "params2 " << thisTasks.size() << endl;

                                     centroidPrinter(thisTasks);
                                     return kernelTester((thisTasks));
                                 });
        futures.push_back(std::move(future));
        cout << "building thread " << i << " done" << endl;
    }

    cout << " about to get results " << endl;

    for (int i = 0; i < futures.size(); i++)
    {
        cout << "here 0" << endl;
        cout << futures[i].valid() << endl;
        auto data = futures[i].get();
        cout << "here 1" << endl;
        cout << "here END OF FUTURE" << endl;
    }
}
