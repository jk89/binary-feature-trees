#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <thread>
#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;
using namespace std;

class Node
{
public:
    Node *parent;
    vector<Node> children;
    vector<int> centroids;
    vector<int> id;
    bool isLeafNode = false;
    Node(vector<int> id, vector<int> centroids, Node *parent)
    {
        this->centroids = centroids;
        this->parent = parent;
        this->id = id;
    }
    json serialise() {}
};

class TrainingNode : public Node
{
public:
    map<int, vector<int>> clusterMembers = {};
    cv::Mat data;
    vector<int> level_data_indices = {};
    void set_cluster_membership(map<int, vector<int>> clusterMembers)
    {
        this->clusterMembers = clusterMembers;
    }
    void process() {
        auto remainingCentroids = getClusterKeys(this->clusterMembers);
        // if (remainingCentoids.size)
    }
    // extensions for model building
    bool finished = false;
    json serialise() {}
    Node toNode() {}
    TrainingNode(cv::Mat &data, vector<int> level_data_indices, vector<int> id, vector<int> centroids, TrainingNode *parent) : Node(id, centroids, parent)
    {
        this->data = data;
        this->level_data_indices = level_data_indices;
        this->centroids = centroids;
        this->parent = parent;
        this->id = id;
    }
};

/*
it=mymap.find('key');
  mymap.erase (it);  
*/

// factory method

// pair of leaf paths (aka word ids) and the root node;
pair<vector<vector<int>>, Node> getModel(char *filename) {} // what about leaf paths
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

void trainModel(cv::Mat &data)
{
    vector<int> indices = getRange(data.rows);
    map<int, vector<int>> lastCluster = kmedoids(data, indices, 8, 12, {0});
    auto centroids = getClusterKeys(lastCluster);
    TrainingNode rootNode = TrainingNode(data, indices, {}, centroids, nullptr);
    rootNode.set_cluster_membership(lastCluster);
}

/*



*/