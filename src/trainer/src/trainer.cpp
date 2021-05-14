#include <iostream>
#include <opencv2/opencv.hpp>
#include <boost/algorithm/string.hpp>
using namespace cv;
using namespace std;

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("usage: DisplayImage.out <Image_Path>\n");
        return -1;
    }
    Mat image;
    image = imread(argv[1], 1);
    if (!image.data)
    {
        printf("No image data \n");
        return -1;
    }
    namedWindow("Display Image", WINDOW_AUTOSIZE);

    string line("test\ttest2\ttest3");
    vector<string> strs;
    boost::split(strs, line, boost::is_any_of("\t"));
    cout << "* size of the vector: " << strs.size() << endl;

    imshow("Display Image", image);
    waitKey(0);
    return 0;
}