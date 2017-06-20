/* 
   Copyright (C) 2013 Erickson R. Nascimento

   THIS SOURCE CODE IS PROVIDED 'AS-IS', WITHOUT ANY EXPRESS OR IMPLIED
   WARRANTY. IN NO EVENT WILL THE AUTHOR BE HELD LIABLE FOR ANY DAMAGES
   ARISING FROM THE USE OF THIS SOFTWARE.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:


   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   Contact: erickson [at] dcc [dot] ufmg [dot] br

*/


#include <vector>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include<opencv2/legacy/legacy.hpp>  
#include <pcl/features/integral_image_normal.h>

#include "brand.h"


void crossCheckMatching( const cv::Mat& descriptors1, const cv::Mat& descriptors2,
                         std::vector<cv::DMatch>& filteredMatches12 )
{
    cv::BFMatcher matcher(cv::NORM_HAMMING);
	//cv::BruteForceMatcher<cv::HammingLUT> matcher;
	std::vector<cv::DMatch> originalMatches12;
	filteredMatches12.clear();
  //  std::vector<std::vector<cv::DMatch> > matches12, matches21;
  //  matcher.knnMatch( descriptors1, descriptors2, matches12, 1 );
    std::vector<cv::DMatch> matches12, matches21;
    matcher.match( descriptors1, descriptors2, matches12);
  //  matcher.knnMatch( descriptors2, descriptors1, matches21, 1 );
    for( size_t m = 0; m < matches12.size(); m++ )
    {
 /*       bool findCrossCheck = false;
        for( size_t fk = 0; fk < matches12[m].size(); fk++ )
        {
            cv::DMatch forward = matches12[m][fk];

            for( size_t bk = 0; bk < matches21[forward.trainIdx].size(); bk++ )
            {
                cv::DMatch backward = matches21[forward.trainIdx][bk];
                if( backward.trainIdx == forward.queryIdx )
                {
                    filteredMatches12.push_back(forward);
                    findCrossCheck = true;
                    break;
                }
            }
            if( findCrossCheck ) break;
        }*/
		filteredMatches12.push_back(matches12[m]);
    }


}

void ransacFilter(std::vector<cv::KeyPoint> & keypoints1, std::vector<cv::KeyPoint> & keypoints2,
	std::vector<cv::DMatch> & matches, std::vector<cv::KeyPoint> & inlier1, std::vector<cv::KeyPoint> & inlier2,
	std::vector<cv::DMatch> & inlierMatch){
	
	std::vector<cv::KeyPoint> alignedKps1, alignedKps2;
	for (size_t i = 0; i < matches.size(); i++) {

	//	std::cout << "ch1" << std::endl;
		alignedKps1.push_back(keypoints1[matches[i].queryIdx]);
//		std::cout << "ch2" << std::endl;
		alignedKps2.push_back(keypoints2[matches[i].trainIdx]);
	}
	//Keypoints to points
	std::vector<cv::Point2f> ps1, ps2;
	for (unsigned i = 0; i < alignedKps1.size(); i++)
		ps1.push_back(alignedKps1[i].pt);

	for (unsigned i = 0; i < alignedKps2.size(); i++)
		ps2.push_back(alignedKps2[i].pt);
	std::vector<uchar> inliersMask(ps1.size());
	cv::findFundamentalMat(ps1, ps2, cv::FM_RANSAC,
		3., 0.99,
		inliersMask);
	//�Ż�ƥ����
	int index = 0;
	for (unsigned i = 0; i < matches.size(); i++) {
		if (inliersMask[i] != 0){
			inlier1.push_back(alignedKps1[i]);
			inlier2.push_back(alignedKps2[i]);
			matches[i].trainIdx = index;
			matches[i].queryIdx = index;
			inlierMatch.push_back(matches[i]);
			index++;
		}
	}
}

void compute_normals(const cv::Mat& cloud, cv::Mat& normals)
{
   pcl::PointCloud<pcl::PointXYZ>::Ptr pcl_cloud( new pcl::PointCloud<pcl::PointXYZ> );

   pcl_cloud->clear();
   pcl_cloud->width     = cloud.cols;
   pcl_cloud->height    = cloud.rows;
   pcl_cloud->points.resize( pcl_cloud->width * pcl_cloud->height);
    
   for(int y = 0; y < cloud.rows; ++y)
   for(int x = 0; x < cloud.cols; ++x)
   {
      pcl_cloud->at(x,y).x = cloud.at<cv::Point3f>(y,x).x;
      pcl_cloud->at(x,y).y = cloud.at<cv::Point3f>(y,x).y;
      pcl_cloud->at(x,y).z = cloud.at<cv::Point3f>(y,x).z;
   }

   pcl::PointCloud<pcl::Normal>::Ptr pcl_normals (new pcl::PointCloud<pcl::Normal>);
   pcl_normals->clear();
   pcl_normals->width  = pcl_cloud->width;
   pcl_normals->height = pcl_cloud->height;
   pcl_normals->points.resize(pcl_cloud->width * pcl_cloud->height);

   pcl::IntegralImageNormalEstimation<pcl::PointXYZ, pcl::Normal> ne;
   ne.setInputCloud( pcl_cloud );

   ne.setNormalSmoothingSize( 5 );
   ne.setNormalEstimationMethod(ne.COVARIANCE_MATRIX);
   ne.compute( *pcl_normals );

   normals.create( cloud.size(), CV_32FC3 );

   for(int y = 0; y < pcl_normals->height; ++y)
   for(int x = 0; x < pcl_normals->width; ++x)
   {
      normals.at<cv::Point3f>(y,x).x = pcl_normals->at(x,y).normal_x;
      normals.at<cv::Point3f>(y,x).y = pcl_normals->at(x,y).normal_y; 
      normals.at<cv::Point3f>(y,x).z = pcl_normals->at(x,y).normal_z; 
   }
}

void create_cloud( const cv::Mat &depth, 
                   float fx, float fy, float cx, float cy, 
                   cv::Mat& cloud )
{
    const float inv_fx = 1.f/fx;
    const float inv_fy = 1.f/fy;

    cloud.create( depth.size(), CV_32FC3 );

    for( int y = 0; y < cloud.rows; y++ )
    {
        cv::Point3f* cloud_ptr = (cv::Point3f*)cloud.ptr(y);
        const uint16_t* depth_prt = (uint16_t*)depth.ptr(y);

        for( int x = 0; x < cloud.cols; x++ )
        {
            float d = (float)depth_prt[x]/5000; // meters
            cloud_ptr[x].x = (x - cx) * d * inv_fx;
            cloud_ptr[x].y = (y - cy) * d * inv_fy;
            cloud_ptr[x].z = d;
        }
    }
}

int main(int argc, char** argv)
{
   if (argc != 9 )
   {
      std::cerr << "Usage: " << argv[0] << " rgb_image1 depth_image1 rgb_image2 depth_image2 fx fy cx cy.\n";
      return(1);
   }

   // loading images and depth information
   cv::Mat rgb1 = cv::imread( argv[1], CV_LOAD_IMAGE_GRAYSCALE );
   cv::Mat depth1 = cv::imread( argv[2], CV_LOAD_IMAGE_ANYDEPTH );

   cv::Mat rgb2 = cv::imread( argv[3], CV_LOAD_IMAGE_GRAYSCALE );
   cv::Mat depth2 = cv::imread( argv[4], CV_LOAD_IMAGE_ANYDEPTH );

   // intrinsics parameters
   float fx = atof(argv[5]);
   float fy = atof(argv[6]);
   float cx = atof(argv[7]);
   float cy = atof(argv[8]);

   // detect keypoint using rgb images
//   cv::Ptr<cv::FeatureDetector> detector = cv::FeatureDetector::create( "STAR" );
 //  cv::Ptr<cv::FeatureDetector> detector = cv::FeatureDetector::create( "FAST" );

 //  cv::FastFeatureDetector fd(60, true);
   std::vector<cv::KeyPoint> keypoints1;
//   detector->detect( rgb1, keypoints1 );
 //  fd.detect(rgb1, keypoints1);

   std::vector<cv::KeyPoint> keypoints2;
//   detector->detect( rgb2, keypoints2 );
//   fd.detect(rgb2, keypoints2);
   // create point cloud and compute normal
   cv::Mat cloud1, normals1;
   create_cloud( depth1, fx, fy, cx, cy, cloud1 );
   compute_normals( cloud1, normals1 );
   
   cv::Mat cloud2, normals2;
   create_cloud( depth2, fx, fy, cx, cy, cloud2 );
   compute_normals( cloud2, normals2 );

   // extract descriptors
   BrandDescriptorExtractor brand(45,32);
   cv::ORB orbdt(500);

  // cv::OrbDescriptorExtractor orbex;
  

   cv::Mat desc1;
//   orbdt(rgb1, cv::Mat(),  keypoints1);
   orbdt.detect(rgb1, keypoints1);
   brand.compute( rgb1, cloud1, normals1, keypoints1, desc1 );
 
// orbdt(rgb1, cv::Mat(), keypoints1, desc1);
   cv::Mat desc2;
//   orbdt(rgb2, cv::Mat(), keypoints2);

   orbdt.detect(rgb2, keypoints2);
   brand.compute( rgb2, cloud2, normals2, keypoints2, desc2 );
//   orbdt(rgb2, cv::Mat(), keypoints2, desc2);

   // matching descriptors
   std::vector<cv::DMatch> matches, inlierMatch;
   std::vector<cv::KeyPoint> inlier1, inlier2;
   crossCheckMatching(desc1, desc2, matches);
   ransacFilter(keypoints1, keypoints2, matches, inlier1, inlier2, inlierMatch);
   cv::Mat outimg;
//   cv::drawMatches(rgb1, keypoints1, rgb2, keypoints2, matches, outimg, cv::Scalar::all(-1), cv::Scalar::all(-1));
   cv::drawMatches(rgb1, inlier1, rgb2, inlier2, inlierMatch, outimg, cv::Scalar::all(-1), cv::Scalar::all(-1));
   cv::imshow("matches", outimg);
   cv::waitKey();

   return 0;
}
