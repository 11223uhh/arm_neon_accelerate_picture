#include<iostream>
#include<fstream> 
#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;
int main(int32_t argc, char **argv)
{
    
    Mat image;
    char* image_path=argv[1];
    char *bin_path=argv[2];
	image = imread(image_path);
    unsigned char * gra_ptr = image.ptr<uchar>(0);
    std::ofstream ofs(bin_path, ofstream::binary);

    ofs.write(reinterpret_cast<const char*>(gra_ptr), sizeof(unsigned char) * image.rows*image.cols*3);
    ofs.close();
    return 0;
    

}