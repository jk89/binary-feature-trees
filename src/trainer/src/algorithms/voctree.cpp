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
    Node *root;
    vector<Node> children;
    vector<int> centroids;
    vector<int> id;
    bool isLeafNode = false;
    Node(vector<int> id, vector<int> centroids, Node *parent, Node *root)
    {
        this->centroids = centroids;
        this->parent = parent;
        this->id = id;
        this->root = root;
    }
    json serialise() {}
};

/*

map<int, vector<int>> kmedoids(cv::Mat &data, vector<int> &indices, int k, int processor_count, vector<int> seeds) {
    cout << "details of m rows" << data.rows << " cols " << data.cols << " tyoe " << data.type() << endl;
    cout << "first row: " << cv::format(data.row(0), cv::Formatter::FMT_PYTHON) << endl;
    cout << "second row: " << cv::format(data.row(1), cv::Formatter::FMT_PYTHON) << endl;
    cout << "Seeding clusters:" << endl;
    vector<int> centroids = seedClusters(data, k, seeds);
    cout << "Centroids: " << endl;
    for (auto i = centroids.begin(); i != centroids.end(); ++i)
    {
        std::cout << *i << ", ";
    }
    cout << endl;

    // begin fitting
    long long bestCost = LLONG_MAX;
    auto bestMembership = optimiseClusterMembership(indices, data, centroids, processor_count);
    bool escape = false;

    int iteration = 0;
    while (escape == false)
    {
        auto optimalSelectionResults = optimiseCentroidSelectionAndComputeCost(data, bestMembership, processor_count);
        auto cost = optimalSelectionResults.first;
        auto clusterMembership = optimalSelectionResults.second;
        centroids = getClusterKeys(clusterMembership);
        cout << "centroids2:" << centroids.size() << endl;
        for (auto i = centroids.begin(); i != centroids.end(); ++i)
        {
            cout << *i << ", ";
        }
        cout << endl;
        cout << " about to opt again" << endl;
        clusterMembership = optimiseClusterMembership(indices, data, centroids, processor_count);
        if (cost < bestCost)
        {
            cout << "Optimsiation improving (currentCost, oldCost)" << cost << " , " << bestCost << endl;
            clusterMembershipPrinter(clusterMembership);
            bestCost = cost;
            bestMembership = clusterMembership;
        }
        else
        {
            escape = true;
        }
        iteration++;
    }
    return bestMembership;
}

*/

class TrainingNode : public Node
{
public:
    map<int, vector<int>> clusterMembers = {};
    int k = 0;
    cv::Mat *data;
    vector<int> level_data_indices = {};
    void set_cluster_membership(map<int, vector<int>> clusterMembers)
    {
        this->clusterMembers = clusterMembers;
    }
    void fit() {
        auto dataPointer = *(this->data);
        // vector<int> centroids = seedClusters(&dataPointer, this->k, this->centroids);
    }
    void process() {
        // take seed centroids and data and create cluster membership
        // clusterMembers = kmedoids(data, this->level_data_indices, 8, 12, {0});
        // auto remainingCentroids = getClusterKeys(this->clusterMembers);
        // if (remainingCentoids.size)
    }
    // extensions for model building
    bool finished = false;
    json serialise() {}
    Node toNode() {}
    TrainingNode(cv::Mat &data, vector<int> level_data_indices, vector<int> id, vector<int> centroids, int k, TrainingNode *parent, TrainingNode *root) : Node(id, centroids, parent, root)
    {
        this->data = &data;
        this->level_data_indices = level_data_indices;
        this->centroids = centroids;
        this->parent = parent;
        this->id = id;
        this->root = root;
        this->k = k;
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
    // map<int, vector<int>> lastCluster = kmedoids(data, indices, 8, 12, {0});
    // auto centroids = getClusterKeys(lastCluster);
    /*TrainingNode rootNode = TrainingNode(data, indices, {}, centroids, nullptr);
    rootNode.set_cluster_membership(lastCluster);*/
}

/*



*/