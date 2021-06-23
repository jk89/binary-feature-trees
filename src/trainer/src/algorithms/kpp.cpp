#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
using namespace std;

int seedKernel(cv::Mat *_data, int currentCentroidIndex, vector<int> centroids, int k) // data, dataIdValue, centroids, k, metric
{
    auto data = *_data;
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

vector<int> seedCentroids(cv::Mat *_data, int _k, vector<int> seeds) // data, k, metric
{
    auto data = *_data;
    cout << "ROUTINE: seed" << endl;

    // largest index
    const int dataLength = data.rows;
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