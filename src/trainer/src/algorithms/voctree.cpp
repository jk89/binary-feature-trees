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
    // bool isLeafNode = false;
    Node(vector<int> id, vector<int> centroids, Node *parent, Node *root)
    {
        this->centroids = centroids;
        this->parent = parent;
        this->id = id;
        this->root = root;
    }
    json serialise() {}
};

class TrainingNode : public Node
{
public:
    TrainingNode *parent;
    TrainingNode *root;
    map<int, vector<int>> clusterMembers = {};
    int k = 0;
    std::shared_ptr<FeatureMatrix> data;
    vector<int> level_data_indices = {};
    vector<TrainingNode> children = {};
    string vocTreeFile;
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
            this->centroids = seedCentroids(this->data, this->k, this->centroids);
        }

        this->clusterMembers = optimiseClusterMembership((this->level_data_indices), this->data, this->centroids, processor_count);
        this->centroids = getClusterKeys(this->clusterMembers);

        bool escape = false;
        int iteration = 0;
        // best cluster memebership up here
        while (escape == false)
        {
            auto optimalSelectionResults = optimiseCentroidSelectionAndComputeClusterCost(this->data, this->clusterMembers, processor_count);
            auto cost = optimalSelectionResults.first;
            auto clusterMembership = optimalSelectionResults.second;
            auto centroids = getClusterKeys(clusterMembership);

            clusterMembership = optimiseClusterMembership((this->level_data_indices), this->data, centroids, processor_count);
            if (cost < this->currentPermutationCost)
            {
                cout << "Cost improving [id | currentCost | oldCost]:[";
                centroidPrinter(this->id);
                cout << " | " << cost << " | " << this->currentPermutationCost << endl;
                centroidPrinter(this->centroids);
                cout << endl;
                this->currentPermutationCost = cost;
                this->clusterMembers = clusterMembership;
                if (this->currentPermutationCost != cost)
                {
                    cout << "wtf man";
                    exit(2);
                }
                centroids = getClusterKeys(this->clusterMembers);
                this->centroids = centroids;
            }
            else
            {
                cout << "cost not improving (cost,best):(" << cost << "|" << this->currentPermutationCost << endl;
                escape = true;
            }
            iteration++;
        }

        // destroy children
        // FIXME THIS IS GOING WRONG
        // CHILDREN ARE BEING DOUBLED OR
        //
        // build children
        // this->children = {};
        for (int i = 0; i < (this->centroids.size()); i++)
        {
            auto centroid_id = this->centroids[i];
            auto level_data_indices = this->clusterMembers[centroid_id];
            vector<int> newId = {};
            for (int k = 0; k < this->id.size(); k++)
            {
                newId.push_back(this->id[k]);
            }
            newId.push_back(i);
            auto child = TrainingNode(this->root->vocTreeFile, this->data, level_data_indices, newId, {0}, this->k, this->processor_count, this, this->root);
            this->children.push_back(child);
        }
        cout << "num child" << this->children.size() << endl;
        cout << "indicies remaining" << this->level_data_indices.size() << endl;
        this->finished = true;
    }
    void process()
    {
        this->fit_level();
        // children are built by now

        clusterMembershipPrinter(this->clusterMembers);

        // deal with children
        for (int i = 0; i < this->children.size(); i++)
        {
            this->children[i].process();
        }
    }
    void save()
    {
        json treeData = this->root->serialise();
        std::string stringTreeData = treeData.dump();
        // save to file
        std::ofstream file(this->root->vocTreeFile);
        file << stringTreeData;
    }
    // extensions for model building
    bool finished = false;
    json serialise()
    {
        json jnode;
        jnode["id"] = this->id;
        jnode["data_indices"] = this->level_data_indices;
        jnode["centroids"] = this->centroids;
        jnode["currentPermutationCost"] = this->currentPermutationCost;
        jnode["k"] = this->k;
        jnode["concurrency"] = this->processor_count;
        jnode["finished"] = this->finished;
        auto children = json::array();
        for (auto child = this->children.begin(); child != this->children.end(); ++child)
        {
            children.push_back(child->serialise());
        }
        jnode["children"] = children;
        jnode["clusterMembers"] = this->clusterMembers;
        return jnode;
    }
    Node toNode() {}
    TrainingNode(string modelName, std::shared_ptr<FeatureMatrix> data, vector<int> level_data_indices, vector<int> id, vector<int> centroids, int k, int processor_count, TrainingNode *parent, TrainingNode *root) : Node(id, centroids, parent, root)
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
        this->root->vocTreeFile = modelName;
        this->processor_count = processor_count;

        if (level_data_indices.size() < k * 2) // fixme what is this value?
        {
            // we are a leaf node
            this->finished = true;
        }
    }
};

TrainingNode deserialise(string modelName, std::shared_ptr<FeatureMatrix> data, json model, TrainingNode *parent, TrainingNode *root)
{
    /*
ns::person p {
    j["name"].get<std::string>(),
    j["address"].get<std::string>(),
    j["age"].get<int>()
};
    */
    cout << "in deserialise" << endl;
    auto level_data_indices = model["data_indices"].get<vector<int>>();
    auto id = model["id"].get<vector<int>>();
    auto centroids = model["centroids"].get<vector<int>>();
    auto currentPermutationCost = model["currentPermutationCost"].get<long long>();
    auto k = model["k"].get<int>();
    auto concurrency = model["concurrency"].get<int>();
    auto children = model["children"];
    auto clusterMembers = model["clusterMembers"].get<map<int, vector<int>>>();

    TrainingNode node = TrainingNode(modelName, data, level_data_indices, id, centroids, k, concurrency, parent, root);

    node.clusterMembers = clusterMembers;
    node.currentPermutationCost = currentPermutationCost;
    node.processor_count = concurrency;

    for (int i = 0; i < children.size(); i++)
    {
        auto childModel = children[i];
        TrainingNode childNode = deserialise(modelName, data, childModel, &node, root);
        node.children.push_back(childNode);
    }

    return node;
}

// factory method

// pair of leaf paths (aka word ids) and the root node;
pair<vector<vector<int>>, Node> getModel(char *filename)
{
} // what about leaf paths
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

json read_jsonfile(string filename)
{
    std::ifstream i(filename.c_str());
    json j;
    i >> j;
    return j;
}

void trainModel(string modelName)
{
    // say model name is: midsized
    // then feature file is midsized_features.yml
    string featureFile = "data/" + modelName + "_features.yml";
    // then the voc tree is midsized_voctree.json
    string vocTree = "data/" + modelName + "_voctree.json";

    // check files exist
    bool featureFileExists = file_exists(featureFile);
    bool treeFileExists = file_exists(vocTree);

    if (featureFileExists == false)
    {
        cout << "Feature file does not exist" << endl;
        exit(1);
    }

    // readFeaturesFromFile
    cv::Mat dataMat = readFeaturesFromFile((char *)featureFile.c_str());

    vector<int> indices = getRange(dataMat.rows);
    auto data = matToVector(dataMat);
    auto sData = std::make_shared<FeatureMatrix>(data);

    // vector<int> origIndicies = {0, 976, 2023, 2843, 549, 3583, 2431, 1445, 3173, 2044, 1283, 4014, 3316, 836, 4734, 628, 3509, 2941, 221, 2556, 3921, 4776, 754, 4374, 4260, 1453, 4703, 2825, 1372, 573, 1776, 985, 2384, 1675, 1820, 1634, 4782, 2872, 3308, 1993, 2427, 3666, 2064, 2534, 2327, 3957, 3639, 1760, 2444, 3435, 4072, 4138, 143, 1483, 4420, 3414, 2926, 3346, 4656, 3127, 3400, 1461, 3380, 1474, 232, 4973, 4932, 127, 1905, 3080, 1670, 3424, 3880, 2868, 2190, 143, 641, 1382, 2644, 2705, 2783, 3664, 4710};

    // kmedoids(sData, &origIndicies, 8, 12, {0}); exit(1);

    // if the voctree file exists
    if (treeFileExists == true)
    {
        exit(1); // this is broken fixme
        json model = read_jsonfile(vocTree);
        TrainingNode rootNode = deserialise(vocTree, sData, model, nullptr, nullptr);
        rootNode.process();
        rootNode.save();
    }
    else
    {
        // {47743, 211873, 225696, 300333, 316793, 324287, 460397, 485301
        TrainingNode rootNode = TrainingNode(vocTree, sData, indices, {}, {0}, 8, 12, nullptr, nullptr);
        rootNode.process();
        rootNode.save();
    }

    cout << " All training finished " << endl;
}

/*



*/