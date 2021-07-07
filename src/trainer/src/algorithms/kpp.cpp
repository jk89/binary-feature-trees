#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
using namespace std;

// FIXME MUST USE DATA INDICIES YOUR SAMPELING FROM GLOBAL DATA SPACE!
int seedKernel(vector<int> dataIndices, std::shared_ptr<FeatureMatrix> _data, int currentCentroidIndex, vector<int> centroids, int k)
{
    // centroids should be local dataIndicies indicies
    auto data = *_data;
    int maxDistance = 0;
    int maxIndex = -1;
    // for the current dataset of features
    for (int r = 0; r < dataIndices.size(); r++)
    {
        /*
        */
        // extract feature from row index
        auto featureGlobalId = dataIndices[r];
        auto currentDatasetFeature = data[featureGlobalId];
        int minDistance = INT_MAX;
        int currentC = -1;
        // for each already selected centroid
        for (int c = 0; c < centroids.size(); c++)
        {
            int currentCentroidLocalDataSetIndex = centroids[c];
            int currentCentroidGlobalDataSetIndex = dataIndices[currentCentroidLocalDataSetIndex];
            auto currentCentroidFeature = data[currentCentroidGlobalDataSetIndex];
            const int distance = hammingDistance(currentDatasetFeature, currentCentroidFeature);
            if (currentCentroidGlobalDataSetIndex == featureGlobalId)
            {
                // distance should be zero
                if (distance != 0)
                {

                    cout << currentCentroidLocalDataSetIndex << "|" << r << endl;
                    cout << currentCentroidGlobalDataSetIndex << "|" << featureGlobalId << endl;
                    cout << distance << endl;
                    centroidPrinter(currentDatasetFeature);
                    cout << endl;
                    cout << currentDatasetFeature.size() << endl;
                    centroidPrinter(currentCentroidFeature);
                    cout << endl;
                    cout << currentCentroidFeature.size() << endl;

                    cout << endl;
                    exit(1);
                }
            }
            minDistance = min(distance, minDistance);
        }
        if (minDistance > maxDistance)
        {

            maxDistance = minDistance;
            maxIndex = r;
            // cout << "maxIndex: " << maxIndex << " minDistance: " << minDistance << endl;
        }
    }

    // cout << "exit:" << maxIndex << endl;

    return maxIndex; // return the best centroid
}

vector<int> seedCentroids(vector<int> dataIndices, std::shared_ptr<FeatureMatrix> _data, int _k, vector<int> seeds) // data, k, metric
{
    // this completely ignores the dataIndicies! FIXME
    auto data = *_data;

    // largest index
    const int dataLength = dataIndices.size();
    vector<int> centroids = seeds;

    int k = _k - seeds.size();

    // add random first centroid index
    if (seeds.size() == 0)
    {
        auto randomSeedLocalIdx = random(0, dataLength - 1);
        centroids.push_back(randomSeedLocalIdx);
    }

    // for the rest of k
    for (int i = 0; i < k; i++)
    {
        const int nextCentroid = seedKernel(dataIndices, _data, i, centroids, k);
        centroids.push_back(nextCentroid);
        // centroidPrinter(centroids);
        // cout << endl;
    }

    // cout << "finished seeds final centroids" << endl;
    // centroidPrinter(centroids);
    //cout << endl;

    return centroids;
}