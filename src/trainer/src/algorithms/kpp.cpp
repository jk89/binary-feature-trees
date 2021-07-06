#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
using namespace std;

int seedKernel(std::shared_ptr<FeatureMatrix> _data, int currentCentroidIndex, vector<int> centroids, int k)
{
    auto data = *_data;
    int maxDistance = 0;
    int maxIndex = -1;
    // for the whole dataset of features
    for (int r = 0; r < data.size(); r++)
    {
        // extract feature from row index
        auto currentDatasetFeature = data[r];
        int minDistance = INT_MAX;
        // for each already selected centroid
        for (int c = 0; c < centroids.size(); c++)
        {
            int currentCentroidDataSetIndex = centroids[c];
            auto currentCentroidFeature = data[currentCentroidDataSetIndex];
            const int distance = hammingDistance(currentDatasetFeature, currentCentroidFeature);
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

vector<int> seedCentroids(std::shared_ptr<FeatureMatrix> _data, int _k, vector<int> seeds) // data, k, metric
{
    // this completely ignores the dataIndicies! FIXME
    auto data = *_data;

    // largest index
    const int dataLength = data.size();
    vector<int> centroids = seeds;

    int k = _k - seeds.size();

    // add random first centroid index
    if (seeds.size() == 0)
    {
        centroids.push_back(random(0, dataLength - 1));
    }

    // for the rest of k
    for (int i = 0; i < k; i++)
    {
        const int nextCentroid = seedKernel(_data, i, centroids, k);
        centroids.push_back(nextCentroid);
    }

    return centroids;
}