#ifndef array
#include <array>
#endif
using namespace std;

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

typedef std::vector<vector<uint8_t>> FeatureMatrix;
