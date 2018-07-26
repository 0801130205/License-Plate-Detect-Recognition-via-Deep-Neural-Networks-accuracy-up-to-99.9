#ifndef TFORM_FCN_H
#define TFORM_FCN_H
#include <opencv2/opencv.hpp>
class matlabexception{};
typedef struct  tfm
{
	cv::Mat forword;//��ǰӳ��
	cv::Mat inv;//����ӳ��
} Tfm;

typedef struct  opt
{
	int order;//��ǰӳ��
	int K;//����ӳ��
} options;
Tfm cp2tform_similarity(cv::Mat src, cv::Mat dst); //srcԴͼ ���� ���κ��ͼ����
cv::Mat itransform(cv::Mat img,Tfm tm,int rows,int cols);

cv::Mat onetone(cv::Mat img, cv::Mat point, int size, int emh, int eh);
#endif