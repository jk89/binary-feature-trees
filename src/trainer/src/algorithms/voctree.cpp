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
    std::shared_ptr<FeatureMatrix> data;
    vector<int> level_data_indices = {};
    vector<TrainingNode> children = {};
    string vocTreeFile;
    // bool isLeafNode = false;
    long long currentPermutationCost = LLONG_MAX;
    int processor_count = 1;
    void set_cluster_membership(map<int, vector<int>> clusterMembers)
    {
        this->clusterMembers = clusterMembers;
    }
    void fit_level()
    {
        cout << "in fit level" << endl;

        if (this->finished == true)
            return;

        // if centroid seeds are incomplete
        if (this->centroids.size() != this->k)
        {
            cout << "seeding" << endl;
            this->centroids = seedCentroids(this->data, this->k, this->centroids);
            centroidPrinter(this->centroids);
        }

        cout << " about to opt cluster membership" << endl;
        this->clusterMembers = optimiseClusterMembership(&(this->level_data_indices), this->data, centroids, processor_count);
        cout << "getting cluster keys" << endl;
        this->centroids = getClusterKeys(this->clusterMembers);
        centroidPrinter(centroids);
        // save
        // this->save();

        bool escape = false;
        int iteration = 0;
        while (escape == false)
        {
            cout << " about to opt centroid selection" << endl;
            auto optimalSelectionResults = optimiseCentroidSelectionAndComputeClusterCost(this->data, this->clusterMembers, processor_count);
            auto cost = optimalSelectionResults.first;
            auto clusterMembership = optimalSelectionResults.second;
            auto centroids = getClusterKeys(clusterMembership);

            cout << " about to opt again" << endl;
            clusterMembership = optimiseClusterMembership(&(this->level_data_indices), this->data, centroids, processor_count);
            cout << "about to check if we made progress" << endl;
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
                // this->save();
            }
            else
            {
                escape = true;
            }
            iteration++;
        }

        cout << "building next level" << endl;

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
            cout << "2" << endl;
            auto id = this->id;
            // PARENT IS NULLPTR U FOOL!

            cout << "3333 " << id.size() << endl; // .size()
            vector<int> newId = this->id;
            cout << "3" << endl;
            newId.push_back(i);
            cout << "4" << endl;
            cout << "building child " << i << endl;
            auto child = TrainingNode(this->root->vocTreeFile, this->data, level_data_indices, newId, {0}, this->k, this->processor_count, this, this->root);
            this->children.push_back(child);
        }

        this->finished = true;
        // save
        // this->save();
    }
    void process()
    {
        this->fit_level();
        cout << "finished this level children are fined by now" << endl;
        // children are built by now

        // deal with children
        for (int i = 0; i < this->children.size(); i++)
        {
            cout << "calling fit for child" << i << endl;
            /*
level_data_indices.size() <= k * 2
            */
            /*if (this->children[i].level_data_indices.size() > (k * 2))
            {
                
            }
            else {
                this->children[i].finished = true;
                this->save();
            }*/
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
        /* json clusterMembers = j_map(this->clusterMembers);
        for (std::map<int, vector<int>>::iterator it = this->clusterMembers.begin(); it != this->clusterMembers.end(); ++it)
        {
            clusterMembers[it->first] = this->clusterMembers[it->first];
        }*/
        jnode["clusterMembers"] = this->clusterMembers; // clusterMembers;
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
        // cout << this->id.size() << endl;
        // exit(1);
        this->root = root;
        this->k = k;
        if (root == nullptr)
        { // if no root is given assume self
            this->root = this;
        }
        this->root->vocTreeFile = modelName;
        this->processor_count = processor_count;

        if (level_data_indices.size() <= k * 2) // fixme what is this value?
        {
            // we are a leaf node
            this->finished = true;
            // this->save();
        }
    }

    // constructor for leaf node
    /*TrainingNode(cv::Mat *data, vector<int> level_data_indices, vector<int> id, vector<int> centroids, int k, int processor_count, TrainingNode *parent, TrainingNode *root) : Node(id, centroids, parent, root)
    {
    }*/
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
    // TrainingNode rootNode = TrainingNode(&data, indices, {}, {0}, 8, 12, nullptr, nullptr);
    auto level_data_indices = model["data_indices"].get<vector<int>>();
    // cout << "level_data_indices" << endl;
    auto id = model["id"].get<vector<int>>();
    // cout << "something wierd is happening" << model["id"].size() << endl;
    // cout << " id size" << id.size() << endl;
    auto centroids = model["centroids"].get<vector<int>>();
    auto currentPermutationCost = model["currentPermutationCost"].get<long long>();
    auto k = model["k"].get<int>();
    auto concurrency = model["concurrency"].get<int>();
    auto children = model["children"];
    auto clusterMembers = model["clusterMembers"].get<map<int, vector<int>>>();

    TrainingNode node = TrainingNode(modelName, data, level_data_indices, id, centroids, k, concurrency, parent, root);
    // cout << " id size" << id.size() << endl;
    // cout << " node id size" << node.id.size() << endl;

    // exit(1);

    node.clusterMembers = clusterMembers;
    node.currentPermutationCost = currentPermutationCost;
    node.processor_count = concurrency;

    cout << "here" << endl;
    cout << "children size " << children.size() << endl;

    for (int i = 0; i < children.size(); i++)
    {
        auto childModel = children[i];
        TrainingNode childNode = deserialise(modelName, data, childModel, &node, root);
        node.children.push_back(childNode);
    }

    /*for (json::iterator it = children.begin(); it != children.end(); ++it)
    {
        auto key = it.key();
        auto childModel = children[key];
        TrainingNode childNode = deserialise(modelName, data, childModel, &node, root);
        node.children.push_back(childNode);
    }*/

    return node;

    // TrainingNode node = TrainingNode(&data, indices, {}, {0}, 8, 12, nullptr, nullptr);
}
/*
    json serialise()
    {
        json jnode;
        jnode["id"] = this->id;
        jnode["data_indices"] = this->level_data_indices;
        jnode["centroids"] = this->centroids;
        jnode["currentPermutationCost"] = this->currentPermutationCost;
        jnode["k"] = this->k;
        jnode["concurrency"] = this->processor_count;
        auto children = json::array();
        for (auto child = this->children.begin(); child != this->children.end(); ++child)
        {
            children.push_back(child->serialise());
        }
        jnode["children"] = children;
        json clusterMembers;
        for (std::map<int, vector<int>>::iterator it = this->clusterMembers.begin(); it != this->clusterMembers.end(); ++it)
        {
            clusterMembers[it->first] = this->clusterMembers[it->first];
        }
        jnode["clusterMembers"] = clusterMembers;
        return jnode;
    }
*/

/*
it=mymap.find('key');
  mymap.erase (it);  
*/

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
    cv::Mat data = readFeaturesFromFile((char *)featureFile.c_str());

    // if the voctree file exists
    /*if (treeFileExists == true)
    {

        // rootNode.process();
    }
    else
    {
        vector<int> indices = getRange(data.rows);
        // {47743, 211873, 225696, 300333, 316793, 324287, 460397, 485301
        TrainingNode rootNode = TrainingNode(vocTree, &data, indices, {}, {0}, 8, 12, nullptr, nullptr);
        // rootNode.process();
    }*/

    //json model = read_jsonfile(vocTree);
    vector<int> indices = getRange(data.rows);
    auto dataVec = matToVector(data);
    // std::shared_ptr<std::vector<std::vector<int>> sharedData; // <std::vector<std::vector<int>>
    // std::shared_ptr<std::vector<std::vector<int>> sPtr;
    auto mSharedPtr = std::make_shared<FeatureMatrix>(dataVec);
    // kmedoids(mSharedPtr, &indices, 8, 1, {0});
    // auto mSharedPtr = std::make_shared<std::vector<uint8_t> >(/* vector constructor arguments*/);

    // concurrentTester(&indices, 1);
    //clusterMembershipPrinter(kmedoids(mSharedPtr, &indices, 8, 1, {0}));
    TrainingNode rootNode = TrainingNode(vocTree, mSharedPtr, indices, {}, {0}, 8, 12, nullptr, nullptr);
    rootNode.process();
    rootNode.save();
    /*       TrainingNode rootNode = TrainingNode(vocTree, mSharedPtr, indices, {}, {0}, 8, 4, nullptr, nullptr);
    //TrainingNode rootNode = deserialise(vocTree, &data, model, nullptr, nullptr);
    rootNode.process();
    rootNode.save();
*/
    // modelName
    // vector<int> indices = getRange(data.rows);
    // TrainingNode rootNode = TrainingNode(vocTree, &data, indices, {}, {0}, 8, 12, nullptr, nullptr);
    // rootNode.process();
    cout << " All training finished " << endl;
}

/*



*/