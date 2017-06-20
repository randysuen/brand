README - BRAND descriptors 


Library Dependencies
----------------------------------------

The BRAND code is based on the OpenCV library and the demo code is based on OpenCV and PCL libraries. It was built by Erickson R. do Nascimento based on his work "On the development of a robust, fast and lightweight keypoint descriptor". The original code has been modified to use ORB keypoint detector instead of STAR detector, and a scale factor for camera parameters is added.

Getting Started
----------------------------------------

Compiling:

mkdir build
cd build
cmake ..
make

Running:

./brand_match_demo rgb1.png depth1.png rgb2.png depth2.png fx fy cx cy s

For the data set provided, fx = fy = 525, cx = 319.5, cy = 239.5, s = 5000.0
