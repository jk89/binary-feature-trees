#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <thread>
#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;
using namespace std;

class ComputeNode
{
public:
    vector<int> id;
    ComputeNode *parent;
    ComputeNode *root;
    cv::Mat feature; // vector<uint8_t>
    int feature_id = -1;
    vector<ComputeNode> children;
    bool isRoot = false;
    bool isLeaf = false;
    string modelName;
    json serialise()
    {
        json jnode;
        jnode["id"] = this->id;
        jnode["feature_id"] = this->feature_id;
        jnode["isLeaf"] = this->isLeaf;
        auto children = json::array();
        for (auto child = this->children.begin(); child != this->children.end(); ++child)
        {
            children.push_back(child->serialise());
        }
        jnode["children"] = children;
        return jnode;
    }
    vector<vector<int>> getLeaves(vector<vector<int>> *_leaves)
    {
        vector<vector<int>> leaves;
        if (_leaves == nullptr)
        {
            leaves = {};
            _leaves = &leaves;
        }
        else
        {
            leaves = *_leaves;
        }
        if (this->isLeaf == false)
        {
            // iterate children
            for (int i = 0; i < this->children.size(); i++)
            {
                this->children[i].getLeaves(_leaves);
            }
        }
        else
        {
            _leaves->push_back(this->id);
        }
        return leaves;
    }
    void pack() {}
    void save()
    {
        if (this->parent == nullptr)
        {
            // we are the route
            json data = this->serialise();
            std::string stringTreeData = data.dump();
            string vocModel = "data/" + this->modelName + "_voccompute.json";
            // save to file
            std::ofstream file(vocModel);
            file << stringTreeData;
        }
        else
        {
            this->root->save();
        }
    }
    ComputeNode()
    {
        this->parent = nullptr;
        this->id = {};
        this->root = this;
        this->isRoot = true;
    }
    ComputeNode(vector<int> id, int feature_id, ComputeNode *parent, ComputeNode *root)
    {
        this->parent = parent;
        this->id = id;
        this->root = root;
        this->feature_id = feature_id;
    }
};

// leaf id's unique
bool leaf_ids_unique(ComputeNode *parent)
{
    std::set<vector<int>> unique_leaf_ids;
    vector<vector<int>> leaves = parent->getLeaves(nullptr);
    for (int i = 0; i < leaves.size(); i++)
    {
        unique_leaf_ids.insert(leaves[i]);
    }
    cout << "total leaves" << leaves.size() << "|" << parent->root->modelName << endl;
    return leaves.size() == unique_leaf_ids.size();
};

bool feature_ids_unique_internal(map<int, int> *duplicates, ComputeNode *parent)
{
    (*duplicates)[parent->feature_id]++;
    for (int i = 0; i < parent->children.size(); i++)
    {
        feature_ids_unique_internal(duplicates, &(parent->children[i]));
    }
};

bool feature_ids_unique(ComputeNode *parent)
{
    map<int, int> duplicatesMap = {};
    feature_ids_unique_internal(&duplicatesMap, parent);
    vector<int> invalidIds = {};
    int total = 0;
    for (auto iter = duplicatesMap.begin(); iter != duplicatesMap.end(); ++iter)
    {
        if (iter->second != 1)
        {
            invalidIds.push_back(iter->first);
        }
        total += iter->second;
    }
    cout << total << " unique features" << endl;
    if (invalidIds.size() == 0)
    {
        return true;
    }
    cout << "we had some duplicated feature ids" << endl;
    centroidPrinter(invalidIds);
    cout << endl;
    return false;
};

ComputeNode trainingNodeToComputeNode2(TrainingNode *parentTrainingNode, ComputeNode *parentComputeNode)
{
    // create parentCN if not exist
    ComputeNode parentCN;
    if (parentComputeNode == nullptr)
    {
        parentCN = ComputeNode();
        parentComputeNode = &parentCN;
    }
    else
    {
        parentCN = *parentComputeNode;
    }

    //deal with centroids
    // centroid case NO centroids
    if (parentTrainingNode->centroids.size() == 1)
    {
        // trainer leaves [0] centroids if this level was
        // skipped as it was too small
        if (parentTrainingNode->level_data_indices.size() == 0)
        {
            // cout << "no level data parent centroid is leaf" << endl;
            // parent is leaf
            parentComputeNode->isLeaf = true;
        }
        else
        {
            // deserialisecout << "had level data need to add leaves" << parentTrainingNode->level_data_indices.size() << endl;
            // add leaves
            for (int i = 0; i < parentTrainingNode->level_data_indices.size(); i++)
            {
                vector<int> leafNodeId = parentComputeNode->id;
                leafNodeId.push_back(i);
                ComputeNode leaf = ComputeNode(leafNodeId, parentTrainingNode->level_data_indices[i], parentComputeNode, parentComputeNode->root); // ComputeNode(vector<int> id, int feature_id, ComputeNode *parent, ComputeNode *root)
                leaf.isLeaf = true;
                parentComputeNode->children.push_back(leaf);
            }
        }
    }
    else
    {
        // we have a layer of valid centroids
        for (int i = 0; i < parentTrainingNode->centroids.size(); i++)
        {
            // create centroid node and add it to parentCN
            auto centroidLocalId = parentTrainingNode->centroids[i];
            auto centroidGlobalId = parentTrainingNode->level_data_indices[centroidLocalId];
            auto featureId = centroidGlobalId;
            auto centroidNodeId = parentComputeNode->id;
            centroidNodeId.push_back(i);
            auto centroidNode = ComputeNode(centroidNodeId, featureId, parentComputeNode, parentComputeNode->root);

            // auto numberOfCentroidChildren = parentTrainingNode->centroids.size();
            trainingNodeToComputeNode2(&parentTrainingNode->children[i], &centroidNode);

            parentComputeNode->children.push_back(centroidNode);
        }

        // for each centroid
        // create centroid CN
        // add children to centroid if they exist
    }

    return parentCN;
};

ComputeNode makeComputeModelFromTrainingModel(TrainingNode *rootTrainingNode)
{
    // create root compute node
    ComputeNode root = trainingNodeToComputeNode2(rootTrainingNode, nullptr);
    return root;
};

ComputeNode deserialiseComputeNode(json model, ComputeNode *parent, ComputeNode *root)
{
    auto id = model["id"].get<vector<int>>();
    auto children = model["children"];
    auto isLeaf = model["isLeaf"].get<bool>();
    auto feature_id = model["feature_id"].get<int>();

    auto node = ComputeNode();
    if (parent != nullptr)
    {
        node = ComputeNode(id, feature_id, parent, root);
    }
    node.isLeaf = isLeaf;

    for (int i = 0; i < children.size(); i++)
    {
        auto childModel = children[i];
        ComputeNode childNode = deserialiseComputeNode(childModel, &node, node.root);
        node.children.push_back(childNode);
    }

    return node;
};

ComputeNode getComputeModelByName(string modelName)
{
    string featureFile = "data/" + modelName + "_features.yml";
    string vocModel = "data/" + modelName + "_voccompute.json";

    bool featureFileExists = file_exists(featureFile);
    bool treeFileExists = file_exists(vocModel);

    if (featureFileExists == false || treeFileExists == false)
    {
        cout << "Feature or tree file does not exist" << endl;
        exit(1);
    }

    json model = read_jsonfile(vocModel);
    cv::Mat dataMat = readFeaturesFromFile((char *)featureFile.c_str());
    vector<int> indices = getRange(dataMat.rows);
    auto data = matToVector(dataMat);
    auto sData = std::make_shared<FeatureMatrix>(data);

    ComputeNode root = deserialiseComputeNode(model, nullptr, nullptr);
    root.modelName = modelName;

    // pack with features
    return root;
}

void trainModelToComputeModel(string modelName)
{
    // say model name is: midsized
    // then feature file is midsized_features.yml
    string featureFile = "data/" + modelName + "_features.yml";
    // then the voc tree is midsized_voctree.json
    string vocTree = "data/" + modelName + "_voctree.json";
    string vocModel = "data/" + modelName + "_voccompute.json";

    // check files exist
    bool featureFileExists = file_exists(featureFile);
    bool treeFileExists = file_exists(vocTree);

    if (featureFileExists == false)
    {
        cout << "Feature file does not exist" << endl;
        exit(1);
    }

    cv::Mat dataMat = readFeaturesFromFile((char *)featureFile.c_str());

    vector<int> indices = getRange(dataMat.rows);
    auto data = matToVector(dataMat);
    auto sData = std::make_shared<FeatureMatrix>(data);

    vector<int> origIndicies = getRange(100);

    // if the voctree file exists
    if (treeFileExists == true)
    {
        // exit(1); // this is broken fixme
        json model = read_jsonfile(vocTree);
        TrainingNode rootNode = deserialise(vocTree, sData, model, nullptr, nullptr);
        auto computeNode = makeComputeModelFromTrainingModel(&rootNode);
        json treeData = computeNode.serialise();
        computeNode.modelName = modelName;
        computeNode.save();
    }
    else
    {
        cout << "Tree model does not exist" << endl;
        exit(1);
    }

    cout << " Packing compute model finished " << endl;
};