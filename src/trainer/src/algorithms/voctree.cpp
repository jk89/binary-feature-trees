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
            this->centroids = seedCentroids(this->level_data_indices, this->data, this->k, this->centroids);
        }

        /*cout << "here1" << endl;
        centroidPrinter(centroids);
        cout << endl;
        clusterMembershipPrinter(this->clusterMembers);*/

        this->clusterMembers = optimiseClusterMembership(this->level_data_indices, this->data, this->centroids, processor_count);

        // this->centroids = getClusterKeys(this->clusterMembers);

        // cout << "here2" << endl;
        // clusterMembershipPrinter(this->clusterMembers);
        bool escape = false;
        int iteration = 0;
        // best cluster memebership up here
        while (escape == false)
        {
            auto optimalSelectionResults = optimiseCentroidSelectionAndComputeClusterCost(this->level_data_indices, this->data, this->clusterMembers, processor_count);
            auto cost = get<0>(optimalSelectionResults);
            auto clusterMembership = get<1>(optimalSelectionResults);
            auto centroids = get<2>(optimalSelectionResults); //getClusterKeys(clusterMembership);

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
                this->centroids = centroids;
                // cout << "22" << endl;
                clusterMembershipPrinter(clusterMembership);
                if (this->currentPermutationCost != cost)
                {
                    cout << "wtf man";
                    exit(2);
                }
                this->save();
                // centroids = getClusterKeys(this->clusterMembers);
                // this->centroids = centroids;
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
            // auto centroid_id = this->centroids[i];
            auto level_data_indices = this->clusterMembers[i]; // centroid_id
            cout << "next level" << level_data_indices.size() << endl;
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

    // vector<int> origIndicies = {914, 180, 192, 624, 708, 840, 948, 1452, 1548, 1812, 1896, 2148, 2244, 2424, 2868, 3588, 3732, 3948, 4056, 4068, 4176, 4188, 4332, 4776, 4824, 61, 313, 385, 829, 877, 1009, 1453, 1573, 1681, 2569, 2641, 2941, 3001, 3121, 3133, 3469, 3553, 3625, 3865, 4045, 4309, 98, 194, 230, 266, 794, 842, 854, 998, 1106, 1166, 1226, 1490, 1538, 1658, 1802, 2222, 3074, 3278, 3362, 3638, 3830, 3854, 3926, 4118, 4430, 4958, 159, 531, 663, 711, 951, 963, 1095, 1131, 1335, 1551, 1563, 1815, 2703, 3171, 3219, 3279, 3675, 4191, 4, 64, 232, 328, 352, 700, 1120, 1576, 1648, 1720, 1768, 2044, 2080, 2164, 2188, 2500, 2548, 2872, 2896, 3088, 3100, 3352, 3508, 3520, 3592, 3760, 3796, 3952, 4000, 4348, 4420, 4540, 4624, 4852, 4960, 485, 845, 965, 1097, 1253, 1313, 1349, 1541, 1577, 1865, 1961, 2465, 2489, 2573, 2777, 2969, 3005, 3281, 3293, 3329, 3509, 3557, 3737, 3845, 3929, 4025, 4121, 4169, 4265, 102, 150, 306, 474, 594, 606, 618, 882, 1230, 1254, 1542, 1998, 2142, 2262, 2310, 2562, 3018, 3102, 3210, 3414, 3606, 3642, 3666, 3906, 4074, 4146, 4374, 4446, 4734, 4782, 4794, 67, 115, 127, 211, 295, 631, 1375, 1399, 1423, 1483, 1543, 1627, 1819, 1903, 1927, 1963, 2359, 2395, 2443, 2575, 2755, 2923, 2959, 2971, 3151, 3187, 3199, 3271, 3583, 3787, 4027, 4231, 4375, 4423, 4579, 4699, 4807, 4819, 236, 404, 464, 620, 836, 1160, 1508, 1604, 1796, 1820, 2456, 2768, 2780, 2852, 2888, 2936, 3524, 3656, 3680, 3968, 4028, 4532, 4568, 4604, 333, 645, 669, 837, 1197, 1353, 1461, 1593, 1761, 1797, 2097, 2661, 2913, 3441, 3705, 3885, 3909, 3921, 4029, 4113, 4233, 4305, 4473, 4497, 4641, 406, 418, 586, 646, 766, 838, 1006, 1054, 1090, 1126, 1474, 1498, 1582, 1714, 1762, 1966, 2074, 2110, 2122, 2254, 2386, 2458, 2578, 2614, 2926, 3118, 3310, 3478, 3514, 3754, 3862, 4018, 4138, 4222, 4354, 4546, 4702, 4930, 143, 335, 551, 851, 1643, 1763, 2195, 2315, 2507, 2519, 2555, 2771, 3011, 3035, 3263, 3551, 3671, 3695, 3755, 3791, 4187, 4283, 4595, 4727, 4739, 4823, 4943};
    vector<int> origIndicies = getRange(100);
    //kmedoids(sData, &origIndicies, 8, 1, {0}); exit(1);

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