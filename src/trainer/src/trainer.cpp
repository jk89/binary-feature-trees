#include <opencv2/opencv.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>
//#include <boost/algorithm/string.hpp>

using namespace cv;
using namespace std;
using namespace boost::filesystem;

namespace fs = boost::filesystem;


const char *filename = "./data/ORBvoc.txt";

/**
 * \brief   Return the filenames of all files that have the specified extension
 *          in the specified directory and all subdirectories.
 */
std::vector<fs::path> get_all(fs::path const & root) // , std::string const & ext
{
    std::vector<fs::path> paths;
    if (fs::exists(root) && fs::is_directory(root))
    {
        for (auto const & entry : fs::directory_iterator(root)) // recursive_directory_iterator
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
        while (std::getline(file, line))
        {
            // using printf() in all tests for consistency
            // printf("%s", line.c_str());
            cout << line.c_str() << endl;
        }
        file.close();
    }
}
