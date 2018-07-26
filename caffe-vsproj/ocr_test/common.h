#pragma once

#define  _CRT_SECURE_NO_WARNINGS
#define OPENCV
#include <caffe/caffe.hpp>




#include "parser.h"


#include"opencv/cv.h"
#include"opencv/highgui.h"
#include"opencv/cxcore.h"



#include"math.h"

#include <stdio.h>
#include <fstream>
#include <iostream>
#include<string>
#include <vector>
#include <windows.h>
#include <process.h>
#include <direct.h> 
#include <cmath>
#include <ctime>
#include <chrono>
#include <random>
#include <memory>


using namespace caffe;  // NOLINT(build/namespaces)
using std::string;



using namespace cv;
using namespace std;




#define showSteps  0
#define showdemo 1
#define USE_OPENCV




int notfound = 2;
int readtype = 0;//1Ϊ����2Ϊ�ַ�

CascadeClassifier car_cascade;
CascadeClassifier char_cascade;
vector<string> file_name, img_path;
vector<string> imgfileNames;



void FastFilter(IplImage *img, double sigma)
{
	int filter_size;

	// Reject unreasonable demands
	if (sigma > 200) sigma = 200;

	// get needed filter size (enforce oddness)
	filter_size = (int)floor(sigma * 6) / 2;
	filter_size = filter_size * 2 + 1;

	// If 3 sigma is less than a pixel, why bother (ie sigma < 2/3)
	if (filter_size < 3) return;

	// Filter, or downsample and recurse
	/*if (filter_size < 10) {

	#ifdef USE_EXACT_SIGMA
	FilterGaussian(img, sigma)
	#else
	cvSmooth( img, img, CV_GAUSSIAN, filter_size, filter_size );
	#endif

	}
	else*/ {
		if (img->width < 2 || img->height < 2) return;

		IplImage* sub_img = cvCreateImage(cvSize(img->width / 2, img->height / 2), img->depth, img->nChannels);

		cvPyrDown(img, sub_img);

		FastFilter(sub_img, sigma / 2.0);

		cvResize(sub_img, img, CV_INTER_LINEAR);

		cvReleaseImage(&sub_img);
	}

}

void MultiScaleRetinex(IplImage *img, int scales, double *weights, double *sigmas, int gain, int offset)
{
	int i;
	double weight;
	IplImage *A, *fA, *fB, *fC;

	// Initialize temp images
	fA = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_32F, img->nChannels);
	fB = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_32F, img->nChannels);
	fC = cvCreateImage(cvSize(img->width, img->height), IPL_DEPTH_32F, img->nChannels);


	// Compute log image
	cvConvert(img, fA);
	cvLog(fA, fB);

	// Normalize according to given weights
	for (i = 0, weight = 0; i < scales; i++)
		weight += weights[i];

	if (weight != 1.0) cvScale(fB, fB, weight);

	// Filter at each scale
	for (i = 0; i < scales; i++) {
		A = cvCloneImage(img);
		FastFilter(A, sigmas[i]);

		cvConvert(A, fA);
		cvLog(fA, fC);
		cvReleaseImage(&A);

		// Compute weighted difference
		cvScale(fC, fC, weights[i]);
		cvSub(fB, fC, fB);
	}

	// Restore
	cvConvertScale(fB, img, gain, offset);

	// Release temp images
	cvReleaseImage(&fA);
	cvReleaseImage(&fB);
	cvReleaseImage(&fC);
}




int init_detect(CascadeClassifier &car_cascade, CascadeClassifier &char_cascade) {

	
	readtype = 2;
	if (!char_cascade.load("cascade12.xml"))
	{
		cerr << "ERROR: Could not load classifier cascade" << endl;
		return -1;
	}
	
	readtype = 1;
	if (!car_cascade.load("cascade_11_plane_card_20160912.xml"))
	{
		cerr << "ERROR: Could not load classifier cascade" << endl;
		return -1;
	}
}

void InsertSort(int a[], int count)
{
	int i, j, temp;
	for (i = 1; i<count; i++)
	{
		temp = a[i];
		j = i - 1;
		while (a[j]>temp && j >= 0)
		{
			a[j + 1] = a[j];
			j--;
		}
		if (j != (i - 1))
			a[j + 1] = temp;
	}
}

int SizeOfRect(const CvRect& rect)   //���
{
	return rect.height*rect.width;
}

CvRect IntersectRect(CvRect result_i, CvRect resmax_j) {    //����
	CvRect rectInter;


	rectInter.x = max(result_i.x, resmax_j.x);
	rectInter.y = max(result_i.y, resmax_j.y);

	int xxx = min((result_i.x + result_i.width), (resmax_j.x + resmax_j.width));
	int yyy = min((result_i.y + result_i.height), (resmax_j.y + resmax_j.height));

	rectInter.width = xxx - rectInter.x;
	rectInter.height = yyy - rectInter.y;

	return rectInter;
}

CvRect UnionRect(CvRect resmax_j, CvRect result_i) {   //����
	CvRect resmax_jj;

	resmax_jj.x = min(result_i.x, resmax_j.x);
	resmax_jj.y = min(result_i.y, resmax_j.y);

	int xxx = max((result_i.x + result_i.width), (resmax_j.x + resmax_j.width));
	int yyy = max((result_i.y + result_i.height), (resmax_j.y + resmax_j.height));

	resmax_jj.width = xxx - resmax_j.x;
	resmax_jj.height = yyy - resmax_j.y;

	return resmax_jj;

}

vector<CvRect> roichoose(vector<CvRect>detectROI, Mat choose_detect_obj) {
	/***************�����δ����ɸѡ*******************/
	int image_width = choose_detect_obj.cols;
	int image_height = choose_detect_obj.rows;
	int judgeheight = image_height*0.3;
	vector<vector<CvRect>> b;//�������Ƶȼ���Ĵ洢λ��
	vector<int> d;//�������Ƽ��洢λ��
				  //	vector<int> shanchu;//��¼��Ҫɾ���ĵ�
	for (int i = detectROI.size() - 1; i > -1; i--)
	{
		if (detectROI[i].height < judgeheight)
			//			shanchu.push_back(i);
			detectROI.erase(detectROI.begin() + i);
	}

	/***************����****************/
	if (showSteps) {
		for (int i = 0; i < detectROI.size(); i++) {
			printf("x: %d \n", detectROI[i].x);
		}
	}
	for (int i = 0; i < detectROI.size(); i++)//ѭ����iָ����ʼ�㣬j�����������
	{
		if (detectROI[i].height < judgeheight)
			continue;
		for (int j = 10; j < (image_width / 7); j++)//jΪ��һ����Χ�ڵĿ������
		{
			/******����*******/



			/******����*******/

			int k = 1;//k����������
			int startpoint = detectROI[i].x;
			int startpoint_y = detectROI[i].y;
			int min_distance = 2 * image_width;       //��¼�������λ�õļ��
			int bestpoint = detectROI.size() + 1; //��¼��ѵ�
			vector<CvRect> grouppoint;//ÿһ����ѵ�Ĵ洢λ��
			grouppoint.push_back(detectROI[i]);

			for (int ii = i + 1; ii < detectROI.size(); ii++)//Ѱ����������ڵ����ŵ�
			{
				if (detectROI[ii].height < judgeheight)
					continue;
				int distance_x = abs(j + startpoint - detectROI[ii].x);
				if (distance_x < 0.5*j)                         //ѡȡxֵ���ʵĵ㣬�Ƚ����������λ�õ�ľ���ѡ��С��
				{
					int distance = (distance_x)*(distance_x)+0.7*(detectROI[ii].y - startpoint_y)*(detectROI[ii].y - startpoint_y);//���Ǹ�����x�����꣬��y�Ĳ�ֵӰ���Ȩ�ؽ���
					distance = sqrt(distance);
					if (distance < min_distance)
					{
						min_distance = distance;
						bestpoint = ii;

					}
				}
			}
			/******����*******/
			//		printf("min_distance=%d", min_distance);

			/*******************************************4-10�޸� �ں����Ŀ�������yֵ��Ԥ��λ��bestpoint_y*************************************/
			while (bestpoint < (detectROI.size() + 1) && min_distance < 2 * j)
			{
				k++;
				grouppoint.push_back(detectROI[bestpoint]);
				startpoint = detectROI[bestpoint].x;//������ʼλ��
				int startpoint_y2 = detectROI[bestpoint].y;//����y2
				min_distance = image_width;
				bestpoint = detectROI.size() + 1;

				for (int ii = i + 1; ii < detectROI.size(); ii++)//Ѱ����������ڵ����ŵ�
				{
					int distance_x = abs(j + startpoint - detectROI[ii].x);
					int best_y = 2 * startpoint_y2 - startpoint_y;//�����е�y����
					int distance_y = abs(detectROI[ii].y - best_y);

					if (distance_x < 0.5*j)                         //ѡȡxֵ���ʵĵ㣬�Ƚ����������λ�õ�ľ���ѡ��С��
					{
						int distance = (distance_x)*(distance_x)+0.7*distance_y*distance_y;//���Ǹ�����x�����꣬��y�Ĳ�ֵӰ���Ȩ�ؽ���
						distance = sqrt(distance);
						if (distance < min_distance)
						{
							min_distance = distance;
							bestpoint = ii;


						}
					}
				}
				startpoint_y = startpoint_y2;//����y1

			}




			if (grouppoint.size() > 3)
			{
				/*if (showSteps)
				{
				for (int testshow = 0; testshow < grouppoint.size(); testshow++){
				printf("x=%d y=%d width=%d height=%d \n", grouppoint[testshow].x, grouppoint[testshow].y, grouppoint[testshow].width, grouppoint[testshow].height);
				}
				getchar();
				}*/

				b.push_back(grouppoint);
				d.push_back(j);
			}

		}
	}


	//������λ�������������������Ƶȼ��򣬽��������ԱȽ�ѡ�����Ž�

	double sum_min = 100000;
	int bestgroup = 0;
	if (b.size() > 2)
	{
		for (int i = 0; i < b.size(); i++)              //����ÿһ����sumֵ,ѡ��bestgroup
		{
			double sum = 0;

			for (int j = 1; j < b[i].size(); j++)
			{
				int dis = d[i];
				sum = sum + (b[i][j].x - b[i][j - 1].x - dis)*(b[i][j].x - b[i][j - 1].x - dis) + 4 * (b[i][j].y - b[i][j - 1].y)*(b[i][j].y - b[i][j - 1].y) + 0.5*(b[i][j].height - b[i][j - 1].height)*(b[i][j].height - b[i][j - 1].height) + 0.5*(b[i][j].width - b[i][j - 1].width)*(b[i][j].width - b[i][j - 1].width);
			}
			sum = sum / b[i].size();
			sum = sum / (1 + pow(b[i].size(), 2));            //����������н�������

															  //				printf("b[i].size=%d sum=%f \n", b[i].size(),sum);
			if (sum < sum_min)
			{
				sum_min = sum;
				bestgroup = i;
			}
		}
		//		printf("size=%d distacne=%d sum_min=%f \n", b[bestgroup].size(), d[bestgroup], sum_min);
		//		return b[bestgroup];
		/*****************��bestgroup�Ļ������ҵ�ǰ��Ŀ�********/
		int bestnumber = b[bestgroup].size();
		int bestx0 = b[bestgroup][0].x;//�������������յ�����
		int besty0 = b[bestgroup][0].y;
		int bestx1 = b[bestgroup][bestnumber - 1].x;
		int besty1 = b[bestgroup][bestnumber - 1].y;
		int bestheight = 0;
		int bestw = 0;
		for (int i = 0; i < bestnumber; i++) {
			bestheight = bestheight + b[bestgroup][0].height;
			bestw = bestw + b[bestgroup][0].width;
		}

		bestheight = bestheight / bestnumber;
		bestw = bestw / bestnumber;
		int bestwidth = d[bestgroup];
		/********************************����ǰ��*****************************/
		int search_startpoint = bestx0;
		int search_starty = besty0;
		int searchpoint = detectROI.size() + 1;
		int search_min_distance = image_width;
		int presize = b[bestgroup].size();

		for (int i = 0; i < detectROI.size(); i++)
		{
			int distance_x = search_startpoint - bestwidth - detectROI[i].x;
			int distance_y = abs(search_starty - detectROI[i].y);
			/***************����****************/

			/*	if (distance_x>-0.8*bestwidth && distance_x < 1.8*bestwidth){
			printf("bestwidth=%d bestheight=%d bestw=%d \n", bestwidth, bestheight, bestw);
			printf("distance_x=%d x=%d \n", distance_x, detectROI[i].x);
			printf("width=%d height=%d \n", detectROI[i].width, detectROI[i].height);
			}*/


			/***************����****************/
			if (distance_x>-0.6*bestwidth && distance_x < 1.8*bestwidth && detectROI[i].height > 0.7*bestheight &&  detectROI[i].height<1.4*bestheight &&   detectROI[i].width  > 0.7*bestw &&  detectROI[i].width < 1.4*bestw && distance_y<0.4*bestheight)  //ѡȡxֵ���ʵĵ㣬�Ƚ����������λ�õ�ľ���ѡ��С�ģ������߶ȿ�ȵ�ɸѡ
			{
				int distance = (distance_x)*(distance_x)+0.7*(besty0 - detectROI[i].y)*(besty0 - detectROI[i].y);//���Ǹ�����x�����꣬��y�Ĳ�ֵӰ���Ȩ�ؽ���
				distance = sqrt(distance);
				if (distance < search_min_distance)
				{
					search_min_distance = distance;
					searchpoint = i;

				}
			}
		}


		while (searchpoint<(detectROI.size() + 1) && search_min_distance<2 * bestwidth)
		{

			b[bestgroup].insert(b[bestgroup].begin(), detectROI[searchpoint]);
			search_startpoint = detectROI[searchpoint].x;//������ʼλ��
			search_starty = detectROI[searchpoint].y;
			search_min_distance = image_width;
			searchpoint = detectROI.size() + 1;

			for (int i = 0; i < detectROI.size(); i++)//Ѱ����������ڵ����ŵ�
			{
				int distance_x = search_startpoint - bestwidth - detectROI[i].x;
				int distance_y = abs(search_starty - detectROI[i].y);
				/***************����****************/
				/*if (distance_x>-0.8*bestwidth && distance_x < 1.8*bestwidth && detectROI[i].height > 0.7*bestheight &&  detectROI[i].height<1.6*bestheight){
				printf("bestwidth=%d bestw=%d bestheight=%d  \n", bestwidth, bestw,bestheight);
				printf("x=%d distance_x=%d width=%d height=%d \n", detectROI[i].x, distance_x, detectROI[i].width, detectROI[i].height);
				}*/

				/***************����****************/
				if (distance_x>-0.6*bestwidth && distance_x < 1.8*bestwidth && detectROI[i].height > 0.7*bestheight &&  detectROI[i].height<1.4*bestheight &&   detectROI[i].width  > 0.7*bestw &&  detectROI[i].width < 1.4*bestw && distance_y<0.4*bestheight)                         //ѡȡxֵ���ʵĵ㣬�Ƚ����������λ�õ�ľ���ѡ��С��
				{

					int distance = (distance_x)*(distance_x)+0.7*(besty0 - detectROI[i].y)*(besty0 - detectROI[i].y);//���Ǹ�����x�����꣬��y�Ĳ�ֵӰ���Ȩ�ؽ���
					distance = sqrt(distance);
					//					    printf("distance=%d \n", distance);
					if (distance < search_min_distance)
					{
						search_min_distance = distance;
						searchpoint = i;
						if (showSteps)
						{
							if (search_min_distance < 2 * bestwidth)
								printf("search_min_distance=%d \n", search_min_distance);
						}
					}
				}
			}
		}


		/********************************���Ӻ��*****************************/
		search_startpoint = bestx1;
		search_starty = besty1;
		searchpoint = detectROI.size() + 1;
		search_min_distance = image_width;


		for (int i = 0; i < detectROI.size(); i++)
		{
			int distance_x = detectROI[i].x - search_startpoint - bestwidth;
			int distance_y = abs(search_starty - detectROI[i].y);
			/***************����****************/

			/*	if (distance_x>-0.8*bestwidth && distance_x < 1.8*bestwidth){
			printf("bestwidth=%d bestheight=%d bestw=%d \n", bestwidth, bestheight, bestw);
			printf("distance_x=%d x=%d \n", distance_x, detectROI[i].x);
			printf("width=%d height=%d \n", detectROI[i].width, detectROI[i].height);
			}*/


			/***************����****************/
			if (distance_x>-0.6*bestwidth && distance_x < 1.8*bestwidth && detectROI[i].height > 0.7*bestheight &&  detectROI[i].height<1.4*bestheight &&   detectROI[i].width  > 0.7*bestw &&  detectROI[i].width < 1.4*bestw && distance_y<0.4*bestheight)  //ѡȡxֵ���ʵĵ㣬�Ƚ����������λ�õ�ľ���ѡ��С�ģ������߶ȿ�ȵ�ɸѡ
			{
				int distance = (distance_x)*(distance_x)+0.7*(besty1 - detectROI[i].y)*(besty1 - detectROI[i].y);//���Ǹ�����x�����꣬��y�Ĳ�ֵӰ���Ȩ�ؽ���
				distance = sqrt(distance);
				if (distance < search_min_distance)
				{
					search_min_distance = distance;
					searchpoint = i;

				}
			}
		}


		while (searchpoint<(detectROI.size() + 1) && search_min_distance<2 * bestwidth)
		{

			b[bestgroup].push_back(detectROI[searchpoint]);
			search_startpoint = detectROI[searchpoint].x;//������ʼλ��
			search_starty = detectROI[searchpoint].y;
			search_min_distance = image_width;
			searchpoint = detectROI.size() + 1;

			for (int i = 0; i < detectROI.size(); i++)//Ѱ����������ڵ����ŵ�
			{
				int distance_x = detectROI[i].x - search_startpoint - bestwidth;
				int distance_y = abs(search_starty - detectROI[i].y);
				/***************����****************/
				/*if (distance_x < 0.8*bestwidth)
				printf("x=%d width=%d height=%d ", detectROI[i].x, detectROI[i].width, detectROI[i].height);*/
				/***************����****************/
				if (distance_x>-0.6*bestwidth && distance_x < 1.8*bestwidth && detectROI[i].height > 0.7*bestheight &&  detectROI[i].height<1.4*bestheight &&   detectROI[i].width  > 0.7*bestwidth &&  detectROI[i].width < 1.4*bestwidth && distance_y<0.4*bestheight)                         //ѡȡxֵ���ʵĵ㣬�Ƚ����������λ�õ�ľ���ѡ��С��
				{
					int distance = (distance_x)*(distance_x)+0.7*(besty1 - detectROI[i].y)*(besty1 - detectROI[i].y);//���Ǹ�����x�����꣬��y�Ĳ�ֵӰ���Ȩ�ؽ���
					distance = sqrt(distance);
					//					printf("distance=%d \n", distance);
					if (distance < search_min_distance)
					{
						search_min_distance = distance;
						searchpoint = i;
						if (showSteps)
						{
							if (search_min_distance < 0.8*bestwidth)
								printf("search_min_distance=%d \n", search_min_distance);
						}
					}
				}
			}
		}
		/*****************��bestgroup�Ļ������ҵ�ǰ��Ŀ����********/

		/********************ѡ��bestgroup�д��ڰ�����ϵ�Ŀ�*********************/
		vector<int> baohan;
		for (int i = 0; i < b[bestgroup].size(); i++)
			for (int j = 0; j < b[bestgroup].size(); j++)
			{
				CvRect recti = b[bestgroup][i];
				CvRect rectj = b[bestgroup][j];
				/***********vector���Ƴ�Ԫ������*************/
				if (rectj.x >= recti.x && rectj.y >= recti.y && rectj.x + rectj.width < recti.x + recti.width && rectj.y + rectj.height < recti.y + recti.height)
				{
					baohan.push_back(j);
				}
			}
		vector<CvRect> group;//ȥ��������ϵ���bestgroup��������
		for (int i = 0; i < b[bestgroup].size(); i++) {
			int kkk = 0;//��kkkΪ0��˵������򲻴��ڱ�������ϵ
			for (int j = 0; j < baohan.size(); j++)
			{
				if (i == baohan[j])
					kkk++;
			}
			if (kkk == 0)
				group.push_back(b[bestgroup][i]);
		}







		int finalsize = b[bestgroup].size();
		if (showSteps) {
			printf("bestx:\n");
			for (int i = 0; i < b[bestgroup].size(); i++)
				printf("%d \n", b[bestgroup][i].x);
			printf("add=%d \n", (finalsize - presize));
		}
		//		return b[bestgroup];
		return group;

	}
	else
	{
		printf(" not found \n");
		notfound = 0;

		return detectROI;
	}

}

vector<CvRect> roinormalization(vector<CvRect>chooseROI, Mat normalization_detect_obj)
{
	vector<CvRect> normal;
	int image_width = normalization_detect_obj.cols;
	int image_height = normalization_detect_obj.rows;
	int width_average = 0;
	int height_average = 0;
	if (chooseROI.size() > 1) {
		for (int i = 0; i < chooseROI.size(); i++)
		{
			width_average += chooseROI[i].width;
			height_average += chooseROI[i].height;
		}
		width_average = width_average / chooseROI.size();
		height_average = height_average / chooseROI.size();
		if (width_average * 5>height_average * 3)
			height_average = width_average * 5 / 3;
		else
			width_average = height_average * 3 / 5;
		for (int i = 0; i < chooseROI.size(); i++)
		{
			CvRect roi_normal;
			roi_normal.x = chooseROI[i].x + 0.5*chooseROI[i].width - 0.5*width_average;
			if (roi_normal.x < 0)
				roi_normal.x = 0;
			roi_normal.y = chooseROI[i].y + 0.5*chooseROI[i].height - 0.5*height_average;
			if (roi_normal.y < 0)
				roi_normal.y = 0;
			roi_normal.width = width_average;
			if (roi_normal.width + roi_normal.x>image_width)
				roi_normal.width = image_width - roi_normal.x;
			roi_normal.height = height_average;
			if (roi_normal.height + roi_normal.y>image_height)
				roi_normal.height = image_height - roi_normal.y;
			normal.push_back(roi_normal);
		}
		return normal;
	}
	else
		return chooseROI;
}

vector<CvRect> roicomplete(vector<CvRect>roinormalization, Mat normalization_detect_obj)
{
	int image_width = normalization_detect_obj.cols;
	int image_height = normalization_detect_obj.rows;
	if (roinormalization.size() > 1)
	{
		int roiwidth = roinormalization[0].width;
		int roiheight = roinormalization[0].height;
		vector<CvRect> com;//comΪС����ʱ�Ĵ洢
		for (int i = 0; i < roinormalization.size(); i++)
		{
			com.push_back(roinormalization[i]);
		}

		if (showSteps)
			cout << "��β�м䲹��ǰ����⵽���ַ�����Ϊ��" << roinormalization.size() << endl;
		int sum = 0;
		int avg_distance_of_chars = 0;
		if (roinormalization.size() > 2) {
			int numSum = 0;

			for (int i = 0; i<roinormalization.size() - 1; i++) { //�����һλ�ų���
				if ((roinormalization[i + 1].x - roinormalization[i].x - roinormalization[i].width) < roiwidth * 0.1) {
					if (showSteps)
						cout << "��" << i << "������ǣ�" << (roinormalization[i + 1].x - (roinormalization[i].x + roinormalization[i].width)) << endl;
					sum += (roinormalization[i + 1].x - (roinormalization[i].x + roinormalization[i].width));
					numSum++;
				}
			}
			//�������һλ��ǰ���forѭ���Ѿ��������һλ�˲��ÿ����ˡ���������
			/*if ((roinormalization[roinormalization.size() - 1].x - roinormalization[roinormalization.size() - 2].x - roinormalization[roinormalization.size() - 2].width) < roiwidth * 0.1)  {
			if (showSteps)
			cout << "��" << roinormalization.size() - 1 << "������ǣ�" << roinormalization[roinormalization.size() - 1].x - roinormalization[roinormalization.size() - 2].x - roinormalization[roinormalization.size() - 2].width << endl;
			sum += (roinormalization[roinormalization.size() - 1].x - roinormalization[roinormalization.size() - 2].x - roinormalization[roinormalization.size() - 2].width);
			numSum++;
			}*/

			avg_distance_of_chars = cvFloor((double)sum / (double)numSum);
		}
		else if (roinormalization.size() == 2) {
			avg_distance_of_chars = roinormalization[1].x - roinormalization[0].x - roinormalization[0].width;
		}
		if (showSteps)
			cout << "ƽ�����Ϊ��" << avg_distance_of_chars << endl;



		for (int i = 1; i < roinormalization.size(); i++)//���м�
		{
			int distance = roinormalization[i].x - roinormalization[i - 1].x - roiwidth;//distanceΪ��ǰ�Ŀ�����Ͻ���ǰһ��������Ͻ�֮���x����룬���ص���Ϊ����
			if (showSteps) {
				cout << "distance " << distance << endl;
				cout << "******************��" << roinormalization[i].x - 2.5*(roiwidth + avg_distance_of_chars) << " " << roinormalization[0].x << " " << 0.1*roiwidth << endl;
			}
			if (((i == 1) || (i == 2)) && (distance > 0.1*roiwidth))
			{
				int j = (distance + 0.2*roiwidth) / (0.8 * roiwidth);//��������0.6-1.4ʱ��һλ��1.4-2.2����λ
				if (showSteps) {
					cout << roinormalization[i].x - roinormalization[i - 1].x - roiwidth << " " << (0.6 * roiwidth) << " " << j << endl;
					cout << (roinormalization[i].x - roinormalization[i - 1].x) % cvRound(0.6*(roiwidth + 2 * avg_distance_of_chars)) << " "
						<< cvRound(0.6*(roiwidth + 2 * avg_distance_of_chars)) << " " << (roiwidth + 2 * avg_distance_of_chars) << endl;
				}
				if ((distance  > 0.6*(roiwidth + 1.5 * avg_distance_of_chars)) && (j == 0))//һ��˵��ƽ�����Ϊ-0.2w,��ǰ�ſ����볬��0.3wʱ���Բ�һλ
				{
					j++;
				}
				if (showSteps) {
					cout << "add 1:" << j << " CvRect" << endl;
				}

				for (int n = 0; n < j; n++) {   //����j = 1�����϶�����ĵ������

					CvPoint centerP;
					centerP.x = roinormalization[i].x - (2 * (j - n) - 1) * (roinormalization[i].x - (roinormalization[i - 1].x + roiwidth)) / (2 * j);
					centerP.y = (roinormalization[i].y + roinormalization[i - 1].y) / 2;

					CvRect Roi;
					Roi.x = centerP.x - roiwidth / 2;
					Roi.y = centerP.y;
					Roi.width = roiwidth;
					Roi.height = roiheight;

					com.push_back(Roi);
				}

			}

			else if ((roinormalization[i].x - 2.7*(roiwidth + avg_distance_of_chars) > roinormalization[0].x) && (distance > 0.01*roiwidth))//���һ����Ƚ�Զ��Ҳ���ǿ������4λ����
			{
				//int j = (roinormalization[i].x - roinormalization[i - 1].x - roiwidth - 2 * avg_distance_of_chars) / (0.6 * roiwidth);//��һ���㷨
				int j = (roinormalization[i].x - roinormalization[i - 1].x - 0.5*roiwidth) / (roiwidth + avg_distance_of_chars);//��һ���㷨
																																//int j = (distance + 0.6*roiwidth) / (0.9 * roiwidth);
				if (showSteps) {
					cout << roinormalization[i].x - roinormalization[i - 1].x - roiwidth - 2 * avg_distance_of_chars << " " << (0.6 * roiwidth) << " " << j << endl;
					cout << (roinormalization[i].x - roinormalization[i - 1].x) % cvRound(0.8*(roiwidth + 2 * avg_distance_of_chars)) << " "
						<< cvRound(0.6*(roiwidth + 2 * avg_distance_of_chars)) << " " << (roiwidth + 2 * avg_distance_of_chars) << endl;
				}
				if (((roinormalization[i].x - roinormalization[i - 1].x - roiwidth)  > 0.6*(roiwidth + 2 * avg_distance_of_chars)) && (j == 0)) {
					j++;
				}
				if (showSteps) {
					cout << "add 2:" << j << " CvRect" << endl;
				}

				for (int n = 0; n < j; n++) {   //����j = 1�����϶�����ĵ������

					CvPoint centerP;
					centerP.x = roinormalization[i].x - (2 * (j - n) - 1) * (roinormalization[i].x - (roinormalization[i - 1].x + roiwidth)) / (2 * j);
					centerP.y = (roinormalization[i].y + roinormalization[i - 1].y) / 2;

					CvRect Roi;
					Roi.x = centerP.x - roiwidth / 2;
					Roi.y = centerP.y;
					Roi.width = roiwidth;
					Roi.height = roiheight;

					com.push_back(Roi);
				}
			}
		}
		if (com.size() == 6)//��ǰ��
		{
			int size = com.size();
			CvRect front, front2;//front2��frontǰ�棨����б�Ҫ��ӵĻ���
			CvRect behind, behind2;//behind2��behind���棨����б�Ҫ��ӵĻ���
			if (roinormalization[1].x - roinormalization[0].x > roiwidth)//ǰ���һ����͵ڶ���������
			{
				if (roinormalization[0].x - 0.8*roiwidth > 0)
					front.x = roinormalization[0].x - 0.8*roiwidth;
				else
					front.x = 0;
				front.y = 2 * roinormalization[0].y - roinormalization[1].y;
				if (front.y < 0)
					front.y = 0;
				if (front.y + roiheight>image_height)
					front.y = image_height - roiheight;
				front.width = roiwidth;
				front.height = roiheight;
				if (roinormalization[0].x - front.x > 0.6*roiwidth)
				{
					com.insert(com.begin(), front);//������ӵ�һ�������� �۲��Ƿ���ӵڶ���

				}
			}
			else //����
			{
				int ddd = roinormalization[1].x - roinormalization[0].x;
				if (ddd > 0.8*roiwidth)
					ddd = 0.8*roiwidth;
				if (roinormalization[0].x - ddd > 0)
					front.x = roinormalization[0].x - ddd;
				else
					front.x = 0;
				front.y = 2 * roinormalization[0].y - roinormalization[1].y;
				if (front.y < 0)
					front.y = 0;
				if (front.y + roiheight>image_height)
					front.y = image_height - roiheight;
				front.width = roiwidth;
				front.height = roiheight;
				if (roinormalization[0].x - front.x>0.6*ddd)
					com.insert(com.begin(), front);
			}
			if (roinormalization[roinormalization.size() - 1].x - roinormalization[roinormalization.size() - 2].x > roiwidth)//������һ�͵����ڶ��Ŀ������
			{
				if (roinormalization[roinormalization.size() - 1].x + 2 * roiwidth < image_width)
					behind.x = roinormalization[roinormalization.size() - 1].x + roiwidth;
				else
					behind.x = image_width - roiwidth;
				behind.y = 2 * roinormalization[roinormalization.size() - 1].y - roinormalization[roinormalization.size() - 2].y;
				if (behind.y < 0)
					behind.y = 0;
				if (behind.y + roiheight>image_height)
					behind.y = image_height - roiheight;
				behind.width = roiwidth;
				behind.height = roiheight;
				if (behind.x - roinormalization[roinormalization.size() - 1].x>0.6*roiwidth)
					com.push_back(behind);
			}
			else//������һ�͵����ڶ��Ŀ������
			{
				int fff = roinormalization[roinormalization.size() - 1].x - roinormalization[roinormalization.size() - 2].x;//�����Ϊfff
				if (roinormalization[roinormalization.size() - 1].x + roiwidth + fff < image_width)//δԽ��
				{
					behind.x = roinormalization[roinormalization.size() - 1].x + fff;
					behind.y = 2 * roinormalization[roinormalization.size() - 1].y - roinormalization[roinormalization.size() - 2].y;
					if (behind.y < 0)
						behind.y = 0;
					if (behind.y + roiheight>image_height)
						behind.y = image_height - roiheight;
					behind.width = roiwidth;
					behind.height = roiheight;
					com.push_back(behind);
				}
				else //Խ��
				{
					behind.width = roiwidth;
					behind.height = roiheight;
					behind.y = 2 * roinormalization[roinormalization.size() - 1].y - roinormalization[roinormalization.size() - 2].y;
					if (behind.y < 0)
						behind.y = 0;
					if (behind.y + roiheight>image_height)
						behind.y = image_height - roiheight;
					behind.x = image_width - roiwidth;
					com.push_back(behind);
				}
			}

		}
		if (com.size() == 5 || com.size() == 4)
		{
			int size = com.size();
			CvRect front, front2;//front2��frontǰ�棨����б�Ҫ��ӵĻ���
			CvRect behind, behind2;//behind2��behind���棨����б�Ҫ��ӵĻ���
			if (roinormalization[1].x - roinormalization[0].x > roiwidth)//ǰ���һ����͵ڶ���������
			{
				if (roinormalization[0].x - 0.8*roiwidth > 0)
					front.x = roinormalization[0].x - 0.8*roiwidth;
				else
					front.x = 0;
				front.y = 2 * roinormalization[0].y - roinormalization[1].y;
				if (front.y < 0)
					front.y = 0;
				if (front.y + roiheight>image_height)
					front.y = image_height - roiheight;
				front.width = roiwidth;
				front.height = roiheight;
				if (roinormalization[0].x - front.x > 0.7*roiwidth)
				{
					com.insert(com.begin(), front);//������ӵ�һ�������� �۲��Ƿ���ӵڶ���
					front2.x = front.x - 0.8*roiwidth;
					front2.y = 2 * front.y - roinormalization[0].y;
					front2.width = roiwidth;
					front2.height = roiheight;
					if (front2.x >= 0 && front2.y >= 0 && front2.y + roiheight< image_height)
						com.insert(com.begin(), front2);

				}
			}
			else //����
			{
				/*int ddd = roinormalization[1].x - roinormalization[0].x;
				if (ddd > 0.8*roiwidth)
				ddd = 0.8*roiwidth;*/
				if (roinormalization[0].x - 1.1*roiwidth > 0)
					front.x = roinormalization[0].x - 1.1*roiwidth;
				else
					front.x = 0;
				front.y = 2 * roinormalization[0].y - roinormalization[1].y;
				if (front.y < 0)
					front.y = 0;
				if (front.y + roiheight>image_height)
					front.y = image_height - roiheight;
				front.width = roiwidth;
				front.height = roiheight;
				if (roinormalization[0].x - front.x > 0.7*roiwidth)
				{
					com.insert(com.begin(), front);
					front2.x = front.x - 0.8*roiwidth;
					front2.y = 2 * front.y - roinormalization[0].y;
					front2.width = roiwidth;
					front2.height = roiheight;
					if (front2.x >= 0 && front2.y >= 0 && front2.y + roiheight< image_height)
						com.insert(com.begin(), front2);
				}
			}
			if (roinormalization[roinormalization.size() - 1].x - roinormalization[roinormalization.size() - 2].x > roiwidth)//������һ�͵����ڶ��Ŀ������
			{
				if (roinormalization[roinormalization.size() - 1].x + 2 * roiwidth < image_width)
					behind.x = roinormalization[roinormalization.size() - 1].x + roiwidth;
				else
					behind.x = image_width - roiwidth;
				behind.y = 2 * roinormalization[roinormalization.size() - 1].y - roinormalization[roinormalization.size() - 2].y;
				if (behind.y < 0)
					behind.y = 0;
				if (behind.y + roiheight > image_height)
					behind.y = image_height - roiheight;
				behind.width = roiwidth;
				behind.height = roiheight;
				if (behind.x - roinormalization[roinormalization.size() - 1].x > 0.95*roiwidth) {
					com.push_back(behind);
					behind2.x = behind.x + roiwidth;
					behind2.y = 2 * behind.y - roinormalization[roinormalization.size() - 1].y;
					behind2.width = roiwidth;
					behind2.height = roiheight;
					if (behind2.x + roiwidth<image_width && behind2.y >= 0 && behind2.y + roiheight < image_height)
						com.push_back(behind2);

				}
			}
			else//������һ�͵����ڶ��Ŀ������
			{
				int fff = roinormalization[roinormalization.size() - 1].x - roinormalization[roinormalization.size() - 2].x;//�����Ϊfff
				if (roinormalization[roinormalization.size() - 1].x + roiwidth + fff < image_width)//δԽ��
				{
					behind.x = roinormalization[roinormalization.size() - 1].x + fff;
					behind.y = 2 * roinormalization[roinormalization.size() - 1].y - roinormalization[roinormalization.size() - 2].y;
					if (behind.y < 0)
						behind.y = 0;
					if (behind.y + roiheight>image_height)
						behind.y = image_height - roiheight;
					behind.width = roiwidth;
					behind.height = roiheight;
					com.push_back(behind);
					behind2.x = behind.x + fff;
					behind2.y = 2 * behind.y - roinormalization[roinormalization.size() - 1].y;
					behind2.width = roiwidth;
					behind2.height = roiheight;
					if (behind2.x + roiwidth<image_width && behind2.y >= 0 && behind2.y + roiheight < image_height)
						com.push_back(behind2);
				}
				else //Խ��
				{
					behind.width = roiwidth;
					behind.height = roiheight;
					behind.y = 2 * roinormalization[roinormalization.size() - 1].y - roinormalization[roinormalization.size() - 2].y;
					if (behind.y < 0)
						behind.y = 0;
					if (behind.y + roiheight>image_height)
						behind.y = image_height - roiheight;
					behind.x = image_width - roiwidth;
					if (behind.x - roinormalization[roinormalization.size() - 1].x > 0.7*roiwidth)
						com.push_back(behind);
				}
			}

		}

		return com;
	}
	else
		return roinormalization;


}

vector<CvRect> roicomplete2(vector<CvRect>roinormalization, Mat normalization_detect_obj)//ʹ����һ���취����ȫ����
{
	int image_width = normalization_detect_obj.cols;
	int image_height = normalization_detect_obj.rows;
	if (roinormalization.size() > 1)
	{
		int roiwidth = roinormalization[0].width;
		int roiheight = roinormalization[0].height;
		vector<CvRect> com;//comΪС����ʱ�Ĵ洢
		for (int i = 0; i < roinormalization.size(); i++)
		{
			com.push_back(roinormalization[i]);
		}
		for (int i = 1; i < roinormalization.size(); i++)//���м�
		{
			int distance = roinormalization[i].x - roinormalization[i - 1].x - roiwidth;
			if (distance>0.4*roiwidth)
			{
				CvRect buchong;
				buchong.x = (roinormalization[i].x + roinormalization[i - 1].x) / 2;
				buchong.y = (roinormalization[i].y + roinormalization[i - 1].y) / 2;
				buchong.width = roiwidth;
				buchong.height = roiheight;
				com.push_back(buchong);

			}
			else if (i > 2 && distance > 0.1*roiwidth)//����ļ�϶����СһЩ
			{
				CvRect buchong2;
				buchong2.x = (roinormalization[i].x + roinormalization[i - 1].x) / 2;
				buchong2.y = (roinormalization[i].y + roinormalization[i - 1].y) / 2;
				buchong2.width = roiwidth;
				buchong2.height = roiheight;
				com.push_back(buchong2);
			}
		}
		//if (com.size() == 5)
		//{
		//	CvRect add1;
		//	CvRect add2;
		//	int carplatepoint = -1;//����������жϳ��Ƶ��λ��
		//	for (int i = 1; i < roinormalization.size(); i++)
		//	{
		//		int pointnumber = 0;

		//	}

		//}
		//if (com.size() == 5 || com.size() == 6)//��ǰ��
		//{
		//	CvRect front;
		//	CvRect behind;
		//	if (roinormalization[1].x - roinormalization[0].x > roiwidth)
		//	{
		//		if (roinormalization[0].x - 0.8*roiwidth > 0)
		//			front.x = roinormalization[0].x - 0.8*roiwidth;
		//		else
		//			front.x = 0;
		//		front.y = 2 * roinormalization[0].y - roinormalization[1].y;
		//		if (front.y < 0)
		//			front.y = 0;
		//		if (front.y + roiheight>image_height)
		//			front.y = image_height - roiheight;
		//		front.width = roiwidth;
		//		front.height = roiheight;
		//		if (roinormalization[0].x - front.x>0.7*roiwidth)
		//			com.insert(com.begin(), front);
		//	}
		//	else
		//	{
		//		int ddd = roinormalization[1].x - roinormalization[0].x;
		//		if (ddd > 0.8*roiwidth)
		//			ddd = 0.8*roiwidth;
		//		if (roinormalization[0].x - ddd > 0)
		//			front.x = roinormalization[0].x - ddd;
		//		else
		//			front.x = 0;
		//		front.y = 2 * roinormalization[0].y - roinormalization[1].y;
		//		if (front.y < 0)
		//			front.y = 0;
		//		if (front.y + roiheight>image_height)
		//			front.y = image_height - roiheight;
		//		front.width = roiwidth;
		//		front.height = roiheight;
		//		if (roinormalization[0].x - front.x>0.8*ddd)
		//			com.insert(com.begin(), front);
		//	}
		//	if (roinormalization[roinormalization.size() - 1].x - roinormalization[roinormalization.size() - 2].x > roiwidth)//������һ�͵����ڶ��Ŀ������
		//	{
		//		if (roinormalization[roinormalization.size() - 1].x + 2 * roiwidth < image_width)
		//			behind.x = roinormalization[roinormalization.size() - 1].x + roiwidth;
		//		else
		//			behind.x = image_width - roiwidth;
		//		behind.y = 2 * roinormalization[roinormalization.size() - 1].y - roinormalization[roinormalization.size() - 2].y;
		//		if (behind.y < 0)
		//			behind.y = 0;
		//		if (behind.y + roiheight>image_height)
		//			behind.y = image_height - roiheight;
		//		behind.width = roiwidth;
		//		behind.height = roiheight;
		//		if (behind.x - roinormalization[roinormalization.size() - 1].x>0.95*roiwidth)
		//			com.push_back(behind);
		//	}
		//	else//������һ�͵����ڶ��Ŀ������
		//	{
		//		int fff = roinormalization[roinormalization.size() - 1].x - roinormalization[roinormalization.size() - 2].x;//�����Ϊfff
		//		if (roinormalization[roinormalization.size() - 1].x + roiwidth + fff < image_width)//δԽ��
		//		{
		//			behind.x = roinormalization[roinormalization.size() - 1].x + fff;
		//			behind.y = 2 * roinormalization[roinormalization.size() - 1].y - roinormalization[roinormalization.size() - 2].y;
		//			if (behind.y < 0)
		//				behind.y = 0;
		//			if (behind.y + roiheight>image_height)
		//				behind.y = image_height - roiheight;
		//			behind.width = roiwidth;
		//			behind.height = roiheight;
		//			com.push_back(behind);
		//		}
		//		else if (com.size() == 6)//Խ��
		//		{
		//			behind.width = roiwidth;
		//			behind.height = roiheight;
		//			behind.y = 2 * roinormalization[roinormalization.size() - 1].y - roinormalization[roinormalization.size() - 2].y;
		//			if (behind.y < 0)
		//				behind.y = 0;
		//			if (behind.y + roiheight>image_height)
		//				behind.y = image_height - roiheight;
		//			behind.x = image_width - roiwidth;
		//			com.push_back(behind);
		//		}
		//	}

		//}
		return com;
	}
	else
		return roinormalization;


}

vector<CvRect>buchong(vector<CvRect>ROI_choose, Mat normalization_detect_obj)
{
	vector<CvRect> qqq;
	int image_width = normalization_detect_obj.cols;
	int image_height = normalization_detect_obj.rows;
	int srcsize = ROI_choose.size();
	for (int i = 1; i < srcsize; i++) {
		qqq.push_back(ROI_choose[i]);
	}
	for (int i = 1; i < srcsize; i++)
	{
		CvRect left;
		CvRect right;
		CvRect up;
		CvRect down;
		left.x = ROI_choose[i].x - 4;
		left.y = ROI_choose[i].y;
		left.width = ROI_choose[i].width;
		left.height = ROI_choose[i].height;
		if (left.x>0)
			qqq.push_back(left);
		//		printf("x=%d,y=%d", left.x, left.y);


		right.x = ROI_choose[i].x + 4;
		right.y = ROI_choose[i].y;
		right.width = ROI_choose[i].width;
		right.height = ROI_choose[i].height;
		if (right.x + right.width<image_width)
			qqq.push_back(right);

		up.x = ROI_choose[i].x;
		up.y = ROI_choose[i].y - 4;
		up.width = ROI_choose[i].width;
		up.height = ROI_choose[i].height;
		if (up.y > 0)
			qqq.push_back(up);

		down.x = ROI_choose[i].x;
		down.y = ROI_choose[i].y + 4;
		down.width = ROI_choose[i].width;
		down.height = ROI_choose[i].height;
		if (down.y + down.height < image_height)
			qqq.push_back(down);
	}
	return qqq;

}

vector<CvRect>hanzibuchong(vector<CvRect>ROI_choose, Mat normalization_detect_obj) {
	vector<CvRect> qqq;
	int image_width = normalization_detect_obj.cols;
	int image_height = normalization_detect_obj.rows;

	qqq.push_back(ROI_choose[0]);

	for (int i = 0; i < 1; i++)
	{
		CvRect left;
		CvRect right;
		CvRect up;
		CvRect down;
		left.x = ROI_choose[i].x - 4;
		left.y = ROI_choose[i].y;
		left.width = ROI_choose[i].width;
		left.height = ROI_choose[i].height;
		if (left.x>0)
			qqq.push_back(left);
		//		printf("x=%d,y=%d", left.x, left.y);


		right.x = ROI_choose[i].x + 4;
		right.y = ROI_choose[i].y;
		right.width = ROI_choose[i].width;
		right.height = ROI_choose[i].height;
		if (right.x + right.width<image_width)
			qqq.push_back(right);

		up.x = ROI_choose[i].x;
		up.y = ROI_choose[i].y - 4;
		up.width = ROI_choose[i].width;
		up.height = ROI_choose[i].height;
		if (up.y > 0)
			qqq.push_back(up);

		down.x = ROI_choose[i].x;
		down.y = ROI_choose[i].y + 4;
		down.width = ROI_choose[i].width;
		down.height = ROI_choose[i].height;
		if (down.y + down.height < image_height)
			qqq.push_back(down);
	}
	return qqq;

}

char* outputplate(int predict) {
	switch (predict)
	{
	case 0:
		//printf("0");
		return "0";
		break;
	case 1:
		//printf("1");
		return "1";
		break;
	case 2:
		//printf("2");
		return "2";
		break;
	case 3:
		//printf("3");
		return "3";
		break;
	case 4:
		//printf("4");
		return "4";
		break;
	case 5:
		//printf("5");
		return "5";
		break;
	case 6:
		//printf("6");
		return "6";
		break;
	case 7:
		//printf("7");
		return "7";
		break;
	case 8:
		//printf("8");
		return "8";
		break;
	case 9:
		//printf("9");
		return "9";
		break;
	case 10:
		//printf("A");
		return "A";
		break;
	case 11:
		//printf("B");
		return "B";
		break;
	case 12:
		//printf("C");
		return "C";
		break;
	case 13:
		//printf("D");
		return "D";
		break;
	case 14:
		//printf("E");
		return "E";
		break;
	case 15:
		//printf("F");
		return "F";
		break;
	case 16:
		//printf("G");
		return "G";
		break;
	case 17:
		//printf("H");
		return "H";
		break;
	case 18:
		//printf("J");
		return "J";
		break;
	case 19:
		//printf("K");
		return "K";
		break;
	case 20:
		//printf("L");
		return "L";
		break;
	case 21:
		//printf("M");
		return "M";
		break;
	case 22:
		//printf("N");
		return "N";
		break;
	case 23:
		//printf("P");
		return "P";
		break;
	case 24:
		//printf("Q");
		return "Q";
		break;
	case 25:
		//printf("R");
		return "R";
		break;
	case 26:
		//printf("S");
		return "S";
		break;
	case 27:
		//printf("T");
		return "T";
		break;
	case 28:
		//printf("U");
		return "U";
		break;
	case 29:
		//printf("V");
		return "V";
		break;
	case 30:
		//		printf("W");
		return "W";
		break;
	case 31:
		//		printf("X");
		return "X";
		break;
	case 32:
		//		printf("Y");
		return "Y";
		break;
	case 33:
		//		printf("Z");
		return "Z";
		break;
	case 34:
		//		printf("ѧ");
		return "ѧ";
		break;
	case 35:
		//		printf("��");
		return "��";
		break;
	case 36:
		//		printf("��");
		return "��";
		break;
	case -1:
		//		printf("?");
		return "?";
		break;

	}
	//	printf(" ");

}

char* outputhanzi(int predict) {
	switch (predict)
	{
	case 0:
		//printf("��");
		return "��";
		break;
	case 1:
		//printf("��");
		return "��";
		break;
	case 2:
		//printf("��");
		return "��";
		break;
	case 3:
		//printf("��");
		return "��";
		break;
	case 4:
		//printf("��");
		return "��";
		break;
	case 5:
		//printf("��");
		return "��";
		break;
	case 6:
		//printf("��");
		return "��";
		break;
	case 7:
		//printf("��");
		return "��";
		break;
	case 8:
		//printf("��");
		return "��";
		break;
	case 9:
		//printf("��");
		return "��";
		break;
	case 10:
		//printf("��");
		return "��";
		break;
	case 11:
		//printf("��");
		return "��";
		break;
	case 12:
		//printf("��");
		return "��";
		break;
	case 13:
		//printf("��");
		return "��";
		break;
	case 14:
		//printf("��");
		return "��";
		break;
	case 15:
		//printf("³");
		return "³";
		break;
	case 16:
		//printf("��");
		return "��";
		break;
	case 17:
		//printf("��");
		return "��";
		break;
	case 18:
		//printf("��");
		return "��";
		break;
	case 19:
		//printf("��");
		return "��";
		break;
	case 20:
		//printf("��");
		return "��";
		break;
	case 21:
		//printf("��");
		return "��";
		break;
	case 22:
		//printf("��");
		return "��";
		break;
	case 23:
		//printf("��");
		return "��";
		break;
	case 24:
		//printf("��");
		return "��";
		break;
	case 25:
		//printf("��");
		return "��";
		break;
	case 26:
		//printf("��");
		return "��";
		break;
	case 27:
		//printf("ԥ");
		return "ԥ";
		break;
	case 28:
		//printf("��");
		return "��";
		break;
	case 29:
		//printf("��");
		return "��";
		break;
	case 30:
		//printf("��");
		return "��";
		break;
	case -1:
		//printf("?");
		return "?";
		break;

	}
	//printf(" ");
}

//BOOL sort_by_x(cv::Point2i point1, cv::Point2i point2) {
//	return (point1.x < point2.x);
//}

void show_choose_step(cv::Mat src, vector<CvRect> ROI_choose, char* windName) {
	if (showSteps)
	{
		cv::Mat shaixuan_obj;
		src.copyTo(shaixuan_obj);

		vector<cv::Scalar> color;
		cv::Scalar magenta = cv::Scalar(255, 0, 255);
		magenta = cv::Scalar(255, 0, 0);// //Draw rectangle around the face			
		color.push_back(magenta);

		magenta = cv::Scalar(0, 255, 0);// //Draw rectangle around the face			
		color.push_back(magenta);

		magenta = cv::Scalar(0, 0, 255);// //Draw rectangle around the face				
		color.push_back(magenta);

		magenta = cv::Scalar(255, 255, 0);// //Draw rectangle around the face			
		color.push_back(magenta);

		magenta = cv::Scalar(255, 0, 255);// //Draw rectangle around the face			
		color.push_back(magenta);

		magenta = cv::Scalar(0, 0, 0);// //Draw rectangle around the face			
		color.push_back(magenta);

		magenta = cv::Scalar(0, 255, 255);// //Draw rectangle around the face			
		color.push_back(magenta);

		magenta = cv::Scalar(100, 13, 200);// //Draw rectangle around the face			
		color.push_back(magenta);

		color.push_back(magenta);
		color.push_back(magenta);
		color.push_back(magenta);
		color.push_back(magenta);

		for (unsigned int j = 0; j < ROI_choose.size(); j++)
		{
			const cv::Rect& single_char_roi = ROI_choose[j];
			printf("x=%d y=%d w=%d h=%d i=%d \n", single_char_roi.x, single_char_roi.y, single_char_roi.width, single_char_roi.height, j);
			cv::Point tl(single_char_roi.x, single_char_roi.y);//Get top-left and bottom-right corner points
			cv::Point br = tl + cv::Point(single_char_roi.width, single_char_roi.height);
			cv::rectangle(shaixuan_obj, tl, br, color[j], 1, 1, 0);
			//detectROI0.push_back(single_char_roi);
		}

		namedWindow(windName, 0);
		imshow(windName, shaixuan_obj);
		//cvWaitKey();
	}
}

void RandomizeIdx(int *idx, int g_cCountTrainingSample)
{
	int i, j;

	srand((unsigned)time(0));

	for (i = 0; i<g_cCountTrainingSample; i++)
	{
		j = int((double)rand() / (double)RAND_MAX*double(g_cCountTrainingSample - 1));

		int temp = idx[i];
		idx[i] = idx[j];
		idx[j] = temp;
	}
}

struct result_
{
	vector<cv::Point2i> point;
	int label = 0;
	cv::Point2i centor = cv::Point2i(0, 0);
	int avg_width = 0;
	int avg_height = 0;
}result_init;

typedef std::pair<string, float> Prediction;
static bool PairCompare(const std::pair<float, int>& lhs,
	const std::pair<float, int>& rhs) {
	return lhs.first > rhs.first;
}

/* Return the indices of the top N values of vector v. */
static std::vector<int> Argmax(const std::vector<float>& v, int N) {
	std::vector<std::pair<float, int> > pairs;
	for (size_t i = 0; i < v.size(); ++i)
		pairs.push_back(std::make_pair(v[i], static_cast<int>(i)));
	std::partial_sort(pairs.begin(), pairs.begin() + N, pairs.end(), PairCompare);

	std::vector<int> result;
	for (int i = 0; i < N; ++i)
		result.push_back(pairs[i].second);
	return result;
}

BOOL sort_by_centor(result_ result1, result_ result2) {
	return (result1.centor.x < result2.centor.x);
}
/* Return the top N predictions. */

/* Load the mean file in binaryproto format. */
BOOL sort_by_x2(cv::Point2i point1, cv::Point2i point2) {
	return (point1.x < point2.x);
}

bool sort_by_x(CvRect obj1, CvRect obj2)
{
	return obj1.x < obj2.x;
}

vector<string> split(const string &s, const string &seperator) {
	vector<string> result;
	typedef string::size_type string_size;
	string_size i = 0;

	while (i != s.size()) {
		//�ҵ��ַ������׸������ڷָ�������ĸ��
		int flag = 0;
		while (i != s.size() && flag == 0) {
			flag = 1;
			for (string_size x = 0; x < seperator.size(); ++x)
				if (s[i] == seperator[x]) {
					++i;
					flag = 0;
					break;
				}
		}

		//�ҵ���һ���ָ������������ָ���֮����ַ���ȡ����
		flag = 0;
		string_size j = i;
		while (j != s.size() && flag == 0) {
			for (string_size x = 0; x < seperator.size(); ++x)
				if (s[j] == seperator[x]) {
					flag = 1;
					break;
				}
			if (flag == 0)
				++j;
		}
		if (i != j) {
			result.push_back(s.substr(i, j - i));
			i = j;
		}
	}
	return result;
}

int outputplate(string predict) {
	if (predict == "0")	return 0;
	else if (predict == "1")	return 1;
	else if (predict == "2")	return 2;
	else if (predict == "3")	return 3;
	else if (predict == "4")	return 4;
	else if (predict == "5")	return 5;
	else if (predict == "6")	return 6;
	else if (predict == "7")	return 7;
	else if (predict == "8")	return 8;
	else if (predict == "9")	return 9;
	else if (predict == "A")	return 10;
	else if (predict == "B")	return 11;
	else if (predict == "C")	return 12;
	else if (predict == "D")	return 13;
	else if (predict == "E")	return 14;
	else if (predict == "F")	return 15;
	else if (predict == "G")	return 16;
	else if (predict == "H")	return 17;
	else if (predict == "J")	return 18;
	else if (predict == "K")	return 19;
	else if (predict == "L")	return 20;
	else if (predict == "M")	return 21;
	else if (predict == "N")	return 22;
	else if (predict == "P")	return 23;
	else if (predict == "Q")	return 24;
	else if (predict == "R")	return 25;
	else if (predict == "S")	return 26;
	else if (predict == "T")	return 27;
	else if (predict == "U")	return 28;
	else if (predict == "V")	return 29;
	else if (predict == "W")	return 30;
	else if (predict == "X")	return 31;
	else if (predict == "Y")	return 32;
	else if (predict == "Z")	return 33;
	else if (predict == "ѧ")	return 34;
	else if (predict == "��")	return 35;
	else if (predict == "��")	return 36;
	else return -1;
}

int outputhanzi(string predict) {
	if (predict == "��")	return 0;
	else if (predict == "��")	return 1;
	else if (predict == "��")	return 2;
	else if (predict == "��")	return 3;
	else if (predict == "��")	return 4;
	else if (predict == "��")	return 5;
	else if (predict == "��")	return 6;
	else if (predict == "��")	return 7;
	else if (predict == "��")	return 8;
	else if (predict == "��")	return 9;
	else if (predict == "��")	return 10;
	else if (predict == "��")	return 11;
	else if (predict == "��")	return 12;
	else if (predict == "��")	return 13;
	else if (predict == "��")	return 14;
	else if (predict == "³")	return 15;
	else if (predict == "��")	return 16;
	else if (predict == "��")	return 17;
	else if (predict == "��")	return 18;
	else if (predict == "��")	return 19;
	else if (predict == "��")	return 20;
	else if (predict == "��")	return 21;
	else if (predict == "��")	return 22;
	else if (predict == "��")	return 23;
	else if (predict == "��")	return 24;
	else if (predict == "��")	return 25;
	else if (predict == "��")	return 26;
	else if (predict == "ԥ")	return 27;
	else if (predict == "��")	return 28;
	else if (predict == "��")	return 29;
	else if (predict == "��")	return 30;
	else return -1;
}

void detectAndDisplay(Mat frame)
{
	std::vector<Rect> faces;
	Mat frame_gray;

	cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
	equalizeHist(frame_gray, frame_gray);

	//-- Detect faces
	/*����imageΪ����ĻҶ�ͼ��objectsΪ�õ����������ľ��ο������飬
	scaleFactorΪÿһ��ͼ��߶��еĳ߶Ȳ�����Ĭ��ֵΪ1.1��
	minNeighbors����Ϊÿһ����������Ӧ�ñ������ڽ�������Ĭ��Ϊ3��
	flags�����µķ�����û���ã���Ŀǰ��haar���������Ǿɰ�ģ�
	CV_HAAR_DO_CANNY_PRUNING����Canny��Ե��������ų�һЩ��Ե���ٻ��ߺܶ��ͼ������
	CV_HAAR_SCALE_IMAGE���ǰ�����������⣬
	CV_HAAR_FIND_BIGGEST_OBJECTֻ����������壬
	CV_HAAR_DO_ROUGH_SEARCHֻ�����Լ�⣩��Ĭ��Ϊ0.
	minSize��maxSize�������Ƶõ���Ŀ������ķ�Χ��*/

	car_cascade.detectMultiScale(frame_gray, faces, 1.15, 2, 0 | CV_HAAR_SCALE_IMAGE, Size(20, 20), Size(170, 170));   // 2. ????


	for (size_t i = 0; i < faces.size(); i++)
	{
		Point center(faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5);
		ellipse(frame, center, Size(faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, Scalar(255, 0, 255), 1, 8, 0);
	}
	//-- Show what you got
	imshow("cascadetest", frame);
}

class Timer {
	using Clock = std::chrono::high_resolution_clock;
public:
	/*! \brief start or restart timer */
	inline void Tic() {
		start_ = Clock::now();
	}
	/*! \brief stop timer */
	inline void Toc() {
		end_ = Clock::now();
	}
	/*! \brief return time in ms */
	inline double Elasped() {
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_ - start_);
		return duration.count();
	}

private:
	Clock::time_point start_, end_;
};

//bool rec_char(caffe::Net& net, cv::Mat src, int& predict, double& loss) {
//	auto input = net.blob_by_name("data");
//	input->Reshape({ 1, 1, 35, 21 });//{64, 100, 1, 1}
//	float *data = input->mutable_cpu_data();//ʶ�������
//	const int n = input->count();
//	//cv::Mat src = cv::imread("img/char4.jpg", 0);
//
//	cv::Mat src2;
//	src.convertTo(src2, CV_32F);
//	cv::resize(src2, src2, cv::Size(21, 35));
//	for (int i = 0; i < n; ++i) {
//		data[i] = src2.at<float>(i / src2.cols, i%src2.cols) / 256;  /* nd(gen);*///תͼ����
//	}
//	// forward
//	/*Timer timer;
//	timer.Tic();*/
//	net.Forward();
//#ifndef US_CPP
//	off_netiof();//close net.txt
//#endif
//				 //timer.Toc();
//				 // visualization
//	auto images = net.blob_by_name("prob");//gconv5 ��������
//										   /*std::cout << net.blob_by_name("prob")->shape_string() << std::endl;*/
//	const int num = images->num();
//	const int channels = images->channels();
//	const int height = images->height();
//	const int width = images->width();
//	const int canvas_len = std::ceil(std::sqrt(num));
//	for (int i = 0; i < channels; i++) {
//		if (i == 0) {
//			loss = images->mutable_cpu_data()[i];//ȡ��Loss
//			predict = 0;
//		}
//		else {
//			if (images->mutable_cpu_data()[i]>loss) {
//				loss = images->mutable_cpu_data()[i];
//				predict = i; //�ó�ʶ����
//			}
//		}
//		/*std::cout << images->mutable_cpu_data()[i] << std::endl;*/
//	}
//	return true;
//}





