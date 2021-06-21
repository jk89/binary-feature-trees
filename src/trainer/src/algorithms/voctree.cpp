#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <thread>

using namespace std;

class Node
{
private:
    Node *parent;
public:
    vector<Node> children;
    vector<int> centroids;
    vector<int> id;
    Node(vector<int> id, vector<int> centroids, Node *parent)
    {
        this->centroids = centroids;
        this->parent = parent;
        this->id = id;
    }
    void serialise() {}
};

// factory method

// pair of leaf paths (aka word ids) and the root node;
pair<vector<vector<int>>,Node> getModel(char *filename) {} // what about leaf paths
// can an empty vector be a valid key index? probably yes

/*
"partitions": 8,
     "centroids": [0, 1, 2, 3];
     "membership": {
         0: [{recursive}]
         1: []
         2: []
         3: []
     }
*/

void trainModel(cv::Mat data)
{
    vector<int> indices = getRange(data.rows);
    map<int, vector<int>> lastCluster = kmedoids(data, indices, 8, 12);
}

/*



*/