#include <opencv2/opencv.hpp>
#include <opencv2/core/mat.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>
#include <stdlib.h> /* atoi */

//#include <boost/algorithm/string.hpp>

// using namespace cv;
using namespace std;
using namespace boost::filesystem;
using namespace std::chrono;

namespace fs = boost::filesystem;

const char *filename = "./data/ORBvoc.txt";

struct compareMats
{
    bool operator()(const cv::Mat &a, const cv::Mat &b) const
    {
        return sum(b - a)[0] > 0;
    }
};

/**
 * \brief   Return the filenames of all files that have the specified extension
 *          in the specified directory and all subdirectories.
 */
std::vector<fs::path> get_all(fs::path const &root) // , std::string const & ext
{
    std::vector<fs::path> paths;
    if (fs::exists(root) && fs::is_directory(root))
    {
        for (auto const &entry : fs::directory_iterator(root)) // recursive_directory_iterator
        {
            if (fs::is_regular_file(entry)) //  && entry.path().extension() == ext
            {
                cout << entry.path().filename() << endl;
                paths.emplace_back(entry.path().filename());
            };
        }
    }

    return paths;
}

int main(int argc, char **argv)
{
    std::ifstream file(filename);
    // file.open(filename, ios::in|ios::out|ios::binary);
    get_all("./data");
    cout << fs::exists(filename) << endl;
    cout << filename << endl;
    cout << file.is_open() << endl;
    if (file.is_open())
    {
        std::string line;
        int idx = 0;
        vector<array<uint8_t, 32>> data = {}; // uint8_t[32]
        // for each line in file
        while (std::getline(file, line))
        {
            if (idx < 1)
            {
                idx++;
                continue;
            };
            std::string cline;
            cline = line.c_str();
            vector<std::string> strs;
            boost::split(strs, line, boost::is_any_of(" ")); // space delimeter
            strs.erase(strs.begin());
            // strip out non-orb data leaving just the orb features
            strs.erase(strs.begin());
            strs.erase(strs.begin() + 34);
            strs.erase(strs.begin() + 34);
            // cout << "* size of the vector: " << strs.size() << endl;
            // decode feature
            // uint8_t feature[32] = {};
            array<uint8_t, 32> feature = {};
            int featureIdx = 0;
            for (auto it = strs.begin(); it != strs.end(); ++it)
            {
                std::string cfeature = *it;
                int icfeature = std::stoi(cfeature);
                uint8_t ui8feature = icfeature;
                feature[featureIdx] = ui8feature;

                // cout << unsigned(feature[featureIdx]) << " ";
                featureIdx++;
            }

            data.push_back(feature);

            // Mat mFeature(32, 1, CV_64FC1, a)

            /*
                double a[] = { 1,2,3,4,5,6, 7,8,9 }; //matrix m0
                double b[] = { 2,4,5,6,8,10,12,13,1 };  //matrix m1
                // rows, cols, format, data
                Mat m0(3, 3, CV_64FC1, a);
                Mat m1(3, 3, CV_64FC1, b);
            */

            idx++;
        }
        file.close();
        cout << "loaded " << data.size() << " ORB vectors" << endl;

        // convert to array
        // vector<uint8_t>

        // uint8_t *a = data.data(); // &data[0];

        // cout << "converted to arr " << endl;
        cout << "done " << endl;

        cv::Mat m = cv::Mat(data.size(), 32, CV_8U); // row col

        memcpy(m.data, data.data(), data.size() * sizeof(array<uint8_t, 32>));

        cout << "details of m rows" << m.rows << " cols " << m.cols << " tyoe " << m.type() << endl;
        cout << "aaa: " << cv::format(m.row(0), cv::Formatter::FMT_PYTHON) << endl;
        cout << "aaa: " << cv::format(m.row(1), cv::Formatter::FMT_PYTHON) << endl;

        std::set<cv::Mat, compareMats> nonDuplicatedPairs;

        for (int i = 0; i < m.rows; ++i)
        {
            nonDuplicatedPairs.insert(m.row(i));
        }

        cout << " unique features " << nonDuplicatedPairs.size() << endl;

        // unique vector
        /*
theVector.assign(theSet.begin(), theSet.end());
        */
        vector<cv::Mat> dedup_vec = {};

        // Iterate till the end of set
        std::set<cv::Mat, compareMats>::iterator it = nonDuplicatedPairs.begin();
        while (it != nonDuplicatedPairs.end())
        {
            // Print the element
            std::cout <<  cv::format((*it), cv::Formatter::FMT_PYTHON)  << " , ";
            //Increment the iterator
            it++;
        }
        // dedup_vec.assign()

        // cout << "M: " << cv::format(nonDuplicatedPairs, cv::Formatter::FMT_PYTHON) << endl;

        // Mat m0(3, 3, CV_64FC1, a);

        // process data.. data
        // Mat m0(3, 3, CV_64FC1, a)
    }

    /*std::chrono::Timer clock; // Timer<milliseconds, steady_clock>

    clock.tick();
    clock.tock();*/

    // cout << "Run time = " << clock.duration().count() << " ms\n";
}
