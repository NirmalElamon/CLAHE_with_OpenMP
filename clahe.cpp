// Name: clahe.cpp
// Author: Nirmal Elamon
// Description:
//  This is an implementation of CLAHE algorithm which will work on any 8 bit or 16 bit images.
//  It will also work on gray scale and RGB images. First the image is divided into small blocks called "tiles".
//  Then each of these blocks are histogram equalized as usual. Contrast limiting is applied to avoid the amplification of the noise.
//  It also uses OpenMP that applies the CLAHE algorithm on a set of images found in a directory using multithreading.
//
// Use:
//    clahe <input image directory name> <output image directory name> <clip limit> <window size>
//
// Example:
//    clahe input output 1 64
//
// Versions
//    1.00 - initial version


#include <stdio.h>
#include <vector>
#include "/usr/local/include/opencv2/imgproc/imgproc.hpp"
#include "/usr/local/include/opencv2/highgui/highgui.hpp"
#include<iostream>
#include<omp.h>
#include<dirent.h>
#include <sys/stat.h>
//#include<chrono>

/*!
 * @brief Checks existence of the specified directory
 * @param sPath - directory path name
 * @return true if directory exists, false otherwise
 */
bool DirExists(const char *sPath)
{
    struct stat info;
    
    if (stat( sPath, &info ) != 0)
    {
        return false;
    }
    
    // This will be true for local disks
    // but will fail for remote directories.
    if (info.st_mode & S_IFDIR)
    {
        return true;
    }
    
    return false;
}



//using namespace std::chrono;
/*
 * @brief Applies the CLAHE algorithm on the image
 *
 * @param[0]  bgr_image - cv::Mat array on which the CLAHE will be applied
 * @param[1]  clip - The value at which the histogram will be clipped
 * @param[2]  window_size - Size of patches or small blocks for dividing the image
 
 * @return     histogram equalized image
 */
cv::Mat clahe_conversion(cv::Mat bgr_image, int clip,int window_size)
    {
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(clip);
    clahe->setTilesGridSize(cv::Size(window_size, window_size));
    cv::Mat dst;
    clahe->apply(bgr_image, dst);
    return dst;
    }


/*
 * @brief Converts the RGB image into Lab colour space followed by extracting the L plane. CLAHE is applied on this L plane by calling the 'clahe_conversion' function. After histogram equalization, the L plane is merged back into Lab space which is then converted to RGB back.
 *
 * @param[0]  bgr_image - cv::Mat array on which the CLAHE will be applied
 * @param[1]  clip - The value at which the histogram will be clipped
 * @param[2]  window_size - Size of patches or small blocks for dividing the image
 
 * @return     histogram equalized RGB image
 */
cv::Mat clahe_rgb(cv::Mat bgr_image,int clip, int window_size)
    {
    cv::Mat lab_image;
    cv::cvtColor(bgr_image, lab_image, CV_BGR2Lab);
    // Extract the L channel
    std::vector<cv::Mat> lab_planes(3);
    cv::split(lab_image, lab_planes);  // now we have the L image in lab_planes[0]
    
    // apply the CLAHE algorithm to the L channel
    cv::Mat dst=clahe_conversion(lab_planes[0],clip,window_size);
    
    // Merge the the color planes back into an Lab image
    dst.copyTo(lab_planes[0]);
    cv::merge(lab_planes, lab_image);
    
    // convert back to RGB
    cv::Mat image_clahe;
    cv::cvtColor(lab_image, image_clahe, CV_Lab2BGR);
    
    return image_clahe;
    }


int main(int argc, char* argv[])
    {
    using namespace std;
    if (argc < 5)
        {
        printf("Useage: clahe <input images directory> <destination directory> <clip limit> <window size> <num_of_threads>\n ");
        return -1;
        }
    
    printf("input images directory: %s\n", argv[1]);
    printf("output images directory: %s\n", argv[2]);
    
        
    //Checking if the input directory exists or not and returning -1 if it doesn't exits
    string input_dir=argv[1];
    string full_input_directory="./" + input_dir;
    if (!DirExists(full_input_directory.c_str()))
        {
        printf("The mentioned input directory doesnot exists!\n");
        return -1;
        }
    
    //Checking if the output directory exists and creating it if not.
    string output_dir=argv[2];
    string full_out_directory="./" + output_dir;
    if (!DirExists(full_out_directory.c_str()))
        {
        printf("The mentioned output directory doesnot exists!\n");
        mkdir(full_out_directory.c_str(),S_IRWXU);
        }
        
    // Parsing the input images directory form the argument and obtaining all the list of images found in that directory
    std::ostringstream input;
    input<<"./"<<argv[1]<<"/*.*";
    
    vector<cv::String> filenames;
    cv::glob (input.str(),filenames);
    
    //Obtaining the number of threads from the user.
    int threads=atoi(argv[5]);
    omp_set_num_threads(threads);
    
        
        
    //high_resolution_clock::time_point t1 = high_resolution_clock::now();
    
    //Using multithreading for applying CLAHE on all the images found in the input directory
    
    #pragma omp parallel for
        {
        // looping through all the images
        for (size_t i=0; i<filenames.size(); i++)
            {
            // READ the image
            cv::Mat bgr_image = cv::imread(filenames[i],-1);
            //Creating the filenames for the output images
            std::ostringstream out;
            std::string base_filename = filenames[i].substr(filenames[i].find_last_of("/\\") + 1);
            out<<"./"<<argv[2]<<"/"<<base_filename;
            
            //convert the clip limit value and window size value for clahe obtained from the command line into integer values
            int clip=atoi(argv[3]);
            int window_size=atoi(argv[4]);
    
            //Determining the type of image
            if (bgr_image.depth()==CV_8U)
                {
                if (bgr_image.channels()==1)
                    {
                    //It is a gray scale 8 bit image
                    printf("The input image: %s is a gray scale 8 bit image\n",base_filename.c_str());
                    cv::Mat dst=clahe_conversion(bgr_image,clip,window_size);
                    cv::imwrite(out.str(), dst);
                    }
                else
                    {
                    //It is an RGB 8 bit image
                    printf("The input image: %s is an RGB 8 bit image\n",base_filename.c_str());
                    cv::Mat dst= clahe_rgb(bgr_image,clip,window_size);
                    cv::imwrite(out.str(), dst);
                    }
                }
    
            else if(bgr_image.depth()==CV_16U)
                {
                //Scaling all the values in the range 0-65535 to 0-255
                double min,max;
                cv::minMaxLoc(bgr_image,&min,&max);
                bgr_image=255*(bgr_image/max);
    
                //Converting the 16 bit to 8 bit
                cv::Mat eightbit_image;
                bgr_image.convertTo(eightbit_image,CV_8U);
        
                if (eightbit_image.channels()==1)
                    {
                    //It is a gray scale 16 bit image
                    printf("The input image: %s is a gray scale 16 bit image\n",base_filename.c_str());
                    cv::Mat dst=clahe_conversion(eightbit_image,clip,window_size);
                    cv::imwrite(out.str(), dst);
                    }
                else
                    {
                    //It is an RGB image 16 bit image
                    printf("The input image: %s is an RGB 16 bit image\n",base_filename.c_str());
                    cv::Mat dst= clahe_rgb(eightbit_image,clip,window_size);
                    cv::imwrite(out.str(), dst);
                    }
                }
            else
                {
                printf("This image format is not supported. It should be either 16 or 8 bit unsigned images");
                }
            }
        }
        //high_resolution_clock::time_point t2 = high_resolution_clock::now();
        //auto duration = duration_cast<microseconds>( t2 - t1 ).count();
        //cout << "The execution time is : " <<duration<<endl;
        
        
    }

