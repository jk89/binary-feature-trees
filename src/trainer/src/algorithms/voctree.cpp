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


map<int, vector<int>> kmedoids(cv::Mat *_data, vector<int> *_indices, int k, int processor_count, vector<int> seeds) {
    auto data = *_data;
    auto indices = *_indices;
    cout << "details of m rows" << data.rows << " cols " << data.cols << " tyoe " << data.type() << endl;
    cout << "first row: " << cv::format(data.row(0), cv::Formatter::FMT_PYTHON) << endl;
    cout << "second row: " << cv::format(data.row(1), cv::Formatter::FMT_PYTHON) << endl;
    cout << "Seeding clusters:" << endl;
    vector<int> centroids = seedClusters(_data, k, seeds);
    cout << "Centroids: " << endl;
    for (auto i = centroids.begin(); i != centroids.end(); ++i)
    {
        std::cout << *i << ", ";
    }
    cout << endl;

    // begin fitting
    long long bestCost = LLONG_MAX;
    auto bestMembership = optimiseClusterMembership(_indices, _data, centroids, processor_count);
    bool escape = false;

    int iteration = 0;
    while (escape == false)
    {
        auto optimalSelectionResults = optimiseCentroidSelectionAndComputeCost(_data, bestMembership, processor_count);
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
        clusterMembership = optimiseClusterMembership(_indices, _data, centroids, processor_count);
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
    TrainingNode *parent;
    TrainingNode *root;
    map<int, vector<int>> clusterMembers = {};
    int k = 0;
    cv::Mat *data;
    vector<int> level_data_indices = {};
    vector<TrainingNode> children = {};
    bool isLeafNode = false;
    long long currentPermutationCost = LLONG_MAX;
    int processor_count = 1;
    void set_cluster_membership(map<int, vector<int>> clusterMembers)
    {
        this->clusterMembers = clusterMembers;
    }
    void fit_level()
    {
        if (this->finished == true)
            return;

        // if centroid seeds are incomplete
        if (this->centroids.size() != this->k)
        {
            this->centroids = seedClusters(this->data, this->k, this->centroids);
            centroidPrinter(this->centroids);
        }

        this->clusterMembers = optimiseClusterMembership(&(this->level_data_indices), this->data, centroids, processor_count);
        this->centroids = getClusterKeys(this->clusterMembers);
        centroidPrinter(centroids);
        // save

        bool escape = false;
        int iteration = 0;
        while (escape == false)
        {
            auto optimalSelectionResults = optimiseCentroidSelectionAndComputeCost(this->data, this->clusterMembers, processor_count);
            auto cost = optimalSelectionResults.first;
            auto clusterMembership = optimalSelectionResults.second;
            auto centroids = getClusterKeys(clusterMembership);

            cout << " about to opt again" << endl;
            clusterMembership = optimiseClusterMembership(&(this->level_data_indices), this->data, centroids, processor_count);
            if (cost < this->currentPermutationCost)
            {
                cout << "Optimsiation improving (currentCost, oldCost)" << cost << " , " << this->currentPermutationCost << endl;
                clusterMembershipPrinter(clusterMembership);
                this->currentPermutationCost = cost;
                this->clusterMembers = clusterMembership;
                centroids = getClusterKeys(this->clusterMembers);
                this->centroids = centroids;
                centroidPrinter(centroids);
                // save
            }
            else
            {
                escape = true;
            }
            iteration++;
        }

        // build children
        for (int i = 0; i < (this->centroids.size()); i++)
        {
            auto centroid_id = this->centroids[i];
            auto level_data_indices = this->clusterMembers[centroid_id];
            if (level_data_indices.size() <= this->k)
            {
                // build leaf node
            }
            vector<int> newId = this->parent->id;
            newId.push_back(i);
            auto child = TrainingNode(this->data, level_data_indices, newId, {0}, this->k, this->processor_count, this, this->root);
            children.push_back(child);
        }

        this->finished = true;
        // save
    }
    void process()
    {
        this->fit_level();

        // deal with children
        for (int i = 0; i < this->children.size(); i++)
        {
            this->children[i].fit_level();
        }
    }
    // extensions for model building
    bool finished = false;
    json serialise() {}
    Node toNode() {}
    TrainingNode(cv::Mat *data, vector<int> level_data_indices, vector<int> id, vector<int> centroids, int k, int processor_count, TrainingNode *parent, TrainingNode *root) : Node(id, centroids, parent, root)
    {
        this->data = data;
        this->level_data_indices = level_data_indices;
        this->centroids = centroids;
        this->parent = parent;
        this->id = id;
        this->root = root;
        this->k = k;
        if (root == nullptr)
        { // if no root is given assume self
            this->root = this;
        }
        this->processor_count = processor_count;
    }

    // constructor for leaf node
    /*TrainingNode(cv::Mat *data, vector<int> level_data_indices, vector<int> id, vector<int> centroids, int k, int processor_count, TrainingNode *parent, TrainingNode *root) : Node(id, centroids, parent, root)
    {
    }*/
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

void trainModel(cv::Mat data)
{
    vector<int> indices = getRange(data.rows);
    // map<int, vector<int>> lastCluster = kmedoids(data, indices, 8, 12, {0});
    // auto centroids = getClusterKeys(lastCluster);
    // cv::Mat* _data = &data;
    TrainingNode rootNode = TrainingNode(&data, indices, {}, {0}, 8, 12, nullptr, nullptr);
    // TrainingNode(_data, indices, {}, {0}, nullptr, nullptr); //
    // TrainingNode()
    // rootNode.set_cluster_membership(lastCluster);*/
}

/*



*/