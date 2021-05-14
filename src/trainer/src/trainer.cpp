#include <opencv2/opencv.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>
#include <stdlib.h> /* atoi */

//#include <boost/algorithm/string.hpp>

// using namespace cv;
using namespace std;
using namespace boost::filesystem;
namespace fs = boost::filesystem;

const char *filename = "./data/ORBvoc.txt";

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
        while (std::getline(file, line))
        {
            if (idx < 1) {
                idx++;
                continue;
            };
            // using printf() in all tests for consistency
            // printf("%s", line.c_str());
            std::string cline;
            cline = line.c_str();
            // cout << cline << endl;
            vector<std::string> strs;
            boost::split(strs, line, boost::is_any_of(" ")); // space delimeter
            strs.erase(strs.begin());
            // strip out non-orb data leaving just the orb features
            strs.erase(strs.begin());
            strs.erase(strs.begin() + 34);
            strs.erase(strs.begin() + 34);
            cout << "* size of the vector: " << strs.size() << endl;
            for (auto it = strs.begin(); it != strs.end(); ++it)
            {
                std::string cfeature = *it;
                // std::cout << cfeature << std::endl;
                int icfeature = std::stoi(cfeature);
                // std::cout << icfeature << endl;
                uint8_t ui8feature = icfeature;
                cout << unsigned(ui8feature) << " ";
            }
            idx++;
        }
        file.close();
    }
}
