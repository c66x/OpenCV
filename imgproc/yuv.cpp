/*M///////////////////////////////////////////////////////////////////////////////////////
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
//  Copyright (C) 2000-2015, Intel Corporation, all rights reserved.
//  Copyright (C) 2009-2011, Willow Garage Inc., all rights reserved.
//  Copyright (C) 2009-2015, NVIDIA Corporation, all rights reserved.
//  Copyright (C) 2010-2013, Advanced Micro Devices, Inc., all rights reserved.
//  Copyright (C) 2015, OpenCV Foundation, all rights reserved.
//  Copyright (C) 2015, Itseez Inc., all rights reserved.
//  Third party copyrights are property of their respective owners.
//
//  Redistribution and use in source and binary forms, with or without modification,
//  are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.

//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.

//   * Neither the names of the copyright holders nor the names of the contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
//  This software is provided by the copyright holders and contributors "as is" and
//  any express or implied warranties, including, but not limited to, the implied
//  warranties of merchantability and fitness for a particular purpose are disclaimed.
//  In no event shall copyright holders or contributors be liable for any direct,
//  indirect, incidental, special, exemplary, or consequential damages
//  (including, but not limited to, procurement of substitute goods or services;
//  loss of use, data, or profits; or business interruption) however caused
//  and on any theory of liability, whether in contract, strict liability,
//  or tort (including negligence or otherwise) arising in any way out of
//  the use of this software, even if advised of the possibility of such damage.
//
//M*/

/*
  yuv.cpp

  Fast YUV conversion functions for opencv; convert directly from Mat to YUV_CAPTURE format

  Copyright (C) Signalogic, 2015

  Revision History:

    Created June 2015, JHB
    Modified July 2015, AKM, moved to folder /opencv/core/
*/

#include "precomp.hpp"
#include <limits>
#include <iostream>

#include "yuv.h"
#include <cv.h>

#ifdef _TI66X

  #include "ia.h"   /* image analytics lib definitions */
  
  extern volatile int testrun;
  extern unsigned int uTILibsConfig;
  extern volatile int cvwidth, cvheight, cvdepth;

#endif

void cv::cvtYuv2Mat(const void* _src, OutputArray _dst, int code) { 

int i, j, width, height;

unsigned int widthStepDst;

#ifdef _TI66X
uint8_t* restrict pRowDstu8;
uint8_t* restrict pYu8;
uint8_t* restrict pUu8;
uint8_t* restrict pVu8;
#else
uint8_t* pRowDstu8;
uint8_t* pYu8;
uint8_t* pUu8;
uint8_t* pVu8;
#endif

   if ( _dst.depth() == CV_8U && _dst.channels() == 3) {

      if (!(uTILibsConfig & IA_OPENCV_USE_FAST_FUNCS)) {  /* if flag not set then use OpenCV slow YUV convert method */

      /* for some reason, pryUp is faster than resize, but pyrDown is not (at least for power-of-2).  so we enable "use pyramids for resize" only in this case.  JHB, Jun2015 */

         unsigned int uTILibsConfig_sav = cvGetTIConfig();
         
         cvSetTIConfig(uTILibsConfig_sav | IA_OPENCV_USE_PYRAMIDS_FOR_RESIZE);
         cvResize(((YUV_CAPTURE*)_src)->cb_half, ((YUV_CAPTURE*)_src)->cb, CV_INTER_CUBIC);  /* note -- CV_INTER_LINEAR is faster */
         cvResize(((YUV_CAPTURE*)_src)->cr_half, ((YUV_CAPTURE*)_src)->cr, CV_INTER_CUBIC);

         cvSetTIConfig(uTILibsConfig_sav);  /* restore TI libs config flags */

         cvMerge(((YUV_CAPTURE*)_src)->y, ((YUV_CAPTURE*)_src)->cr, ((YUV_CAPTURE*)_src)->cb, NULL, ((YUV_CAPTURE*)_src)->ycrcb);

         cv::Mat ycrcb = cv::cvarrToMat(((YUV_CAPTURE*)_src)->ycrcb);
         cvtColor(ycrcb, _dst, CV_YCrCb2BGR);

         return;
      }

      Size sz = _dst.size();
      _dst.create(sz, CV_MAKE_TYPE(_dst.depth(), _dst.channels()));
      Mat dst = _dst.getMat();

      width = sz.width;
      height = sz.height;
      widthStepDst = dst.step;

      pYu8 = (uint8_t*)((YUV_CAPTURE*)_src)->y->imageData;
      pUu8 = (uint8_t*)((YUV_CAPTURE*)_src)->cb_half->imageData;
      pVu8 = (uint8_t*)((YUV_CAPTURE*)_src)->cr_half->imageData;

#ifdef _TI66X
      #pragma MUST_ITERATE(8,,8)
#endif

      for (j=0; j<height; j++) {

         pRowDstu8 = (uint8_t*)((unsigned int)(dst.data + j*widthStepDst));

        /* Note - ITU-R BT601 coefficients are commented.  We're using JPEG (full range) coefficients */

#ifdef _TI66X
         #pragma MUST_ITERATE(64,,8)
#endif
         for (i=0; i<width; i++) {

//            Y = 298*((int)*pYu8++ - 16);
            int Y = 256*(int)*pYu8++;

         /* B */

//            int iVal = (Y + 516*((int)*pUu8-128)) >> 8;
            int iVal = (Y + 452*((int)*pUu8 - 128)) >> 8;
//            _sadd(iVal, 0);  /* possible option:  use saturation intrinsics to perform min <= x <= max type of checks.  JHB Jun 2015 */
            if (iVal < 0) iVal = 0;
            if (iVal > 255) iVal = 255;
            pRowDstu8[3*i] = iVal;

         /* G */

//            iVal = (Y + -100*((int)*pUu8-128) + -208*((int)*pVu8-128)) >> 8;
            iVal = (Y + -88*((int)*pUu8 - 128) + -182*((int)*pVu8 - 128)) >> 8;
            if (iVal < 0) iVal = 0;
            if (iVal > 255) iVal = 255;
            pRowDstu8[3*i+1] = iVal;

         /* R */

//            iVal = (Y + 408*((int)*pVu8-128)) >> 8;
            iVal = (Y + 358*((int)*pVu8 - 128)) >> 8;
            if (iVal < 0) iVal = 0;
            if (iVal > 255) iVal = 255;
            pRowDstu8[3*i+2] = iVal; 

            if (i & 1) {  /* note, this is faster than code such as pUu8 += (i & 1); */

               pUu8++;
               pVu8++;
            }
         }

         if ((j & 1) == 0) {
            pUu8 -= width/2;
            pVu8 -= width/2;
         }
      }
   }
}

void cv::cvtMat2Yuv(InputArray _src, void* _dst, int code) { 

Mat src = _src.getMat();
Size sz = src.size();

int i, j, width, height;

unsigned int widthStepSrc;

#ifdef _TI66X
uint8_t* restrict pRowSrcu8;
uint8_t* restrict pYu8;
uint8_t* restrict pUu8;
uint8_t* restrict pVu8;
#else
uint8_t* pRowSrcu8;
uint8_t* pYu8;
uint8_t* pUu8;
uint8_t* pVu8;
#endif

   if (src.depth() == CV_8U && src.channels() == 3) {

       if (!(uTILibsConfig & IA_OPENCV_USE_FAST_FUNCS)) {  /* if flag not set then use OpenCV slow YUV convert method */

         IplImage ycrcb = src;
         cvCvtColor((const CvArr*)&ycrcb, ((YUV_CAPTURE*)_dst)->ycrcb, CV_BGR2YCrCb);

         cvSplit(((YUV_CAPTURE*)_dst)->ycrcb, ((YUV_CAPTURE*)_dst)->y, ((YUV_CAPTURE*)_dst)->cr, ((YUV_CAPTURE*)_dst)->cb, NULL);
         cvResize(((YUV_CAPTURE*)_dst)->cr, ((YUV_CAPTURE*)_dst)->cr_half, CV_INTER_AREA);
         cvResize(((YUV_CAPTURE*)_dst)->cb, ((YUV_CAPTURE*)_dst)->cb_half, CV_INTER_AREA);

         return;
      }       

      width = sz.width;
      height = sz.height;

      widthStepSrc = src.step;

      pYu8 = (uint8_t*)((YUV_CAPTURE*)_dst)->y->imageData;
      pUu8 = (uint8_t*)((YUV_CAPTURE*)_dst)->cb_half->imageData;
      pVu8 = (uint8_t*)((YUV_CAPTURE*)_dst)->cr_half->imageData;

      _nassert(((int)widthStepSrc % 8) == 0);
      _nassert(((int)src.data % 8) == 0);

#ifdef _TI66X
      #pragma MUST_ITERATE(8,,8)
#endif

      for (j=0; j<height; j++) {

         pRowSrcu8 = (uint8_t*)((unsigned int)(src.data + j*widthStepSrc));

      /* Note - ITU-R BT601 coefficients are commented.  We're using JPEG "full range" coefficients (full range means resulting values range from 0..255.  ITU_R range is 16 to 235) */

      /* Y plane */

         _nassert(((int)pYu8 % 8) == 0);
         _nassert(((int)pUu8 % 8) == 0);
         _nassert(((int)pVu8 % 8) == 0);
         _nassert(((int)pRowSrcu8 % 8) == 0);

#ifdef _TI66X
         #pragma MUST_ITERATE(64,,8)
#endif
         for (i=0; i<width; i++) {

#if 0
//            *pYu8++ = ((66*pRowSrcu8[3*i+2] + 129*pRowSrcu8[3*i+1] + 25*pRowSrcu8[3*i] ) >> 8) + 16;
            *pYu8++ = (76*pRowSrcu8[3*i+2] + 150*pRowSrcu8[3*i+1] + 29*pRowSrcu8[3*i] ) >> 8;

         /* U+V planes (Cr, Cb) */

            if ((j & 1) & !(i & 1)) {

//               *pUu8++ = (( -38*pRowSrcu8[3*i+2] - 74*pRowSrcu8[3*i+1] + 112*pRowSrcu8[3*i] ) >> 8) + 128;
               *pUu8++ = (( -43*pRowSrcu8[3*i+2] - 84*pRowSrcu8[3*i+1] + 128*pRowSrcu8[3*i] ) >> 8) + 128;
//               *pVu8++ = (( 112*pRowSrcu8[3*i+2] -94*pRowSrcu8[3*i+1] - 18*pRowSrcu8[3*i] ) >> 8) + 128;
               *pVu8++ = (( 128*pRowSrcu8[3*i+2] - 107*pRowSrcu8[3*i+1] - 21*pRowSrcu8[3*i] ) >> 8) + 128;
            }
#else
            *pYu8++ = (29*pRowSrcu8[3*i] + 150*pRowSrcu8[3*i+1] + 76*pRowSrcu8[3*i+2] ) >> 8;

         /* U+V planes (Cr, Cb) */

            if ((j & 1) & !(i & 1)) {

               *pUu8++ = (( 128*pRowSrcu8[3*i] - 84*pRowSrcu8[3*i+1] + -43*pRowSrcu8[3*i+2] ) >> 8) + 128;
               *pVu8++ = (( -21*pRowSrcu8[3*i] - 107*pRowSrcu8[3*i+1] + 128*pRowSrcu8[3*i+2] ) >> 8) + 128;
            }
#endif

         }

#if 0
         long long im1_p7_p6_p5_p4_p3_p2_p1_p0, im2_p7_p6_p5_p4_p3_p2_p1_p0;
         int im1_p7_p6_p5_p4, im1_p3_p2_p1_p0, im2_p7_p6_p5_p4, im2_p3_p2_p1_p0;
         int res_p7_p6_p5_p4, res_p3_p2_p1_p0;

         for (int i=0; i<width*nChannels/8; i++) {  /* use SIMD intrinsics for uint8 values to process 8 pixels per iteration */

         /* read 8 pixels */

            im1_p7_p6_p5_p4_p3_p2_p1_p0 = _amem8(pRowSrcu8);
            pRowSrcu8 += 8;

         /* convert to int */

            im1_p3_p2_p1_p0 = _loll(im1_p7_p6_p5_p4_p3_p2_p1_p0);
            im1_p7_p6_p5_p4 = _hill(im1_p7_p6_p5_p4_p3_p2_p1_p0);


         /* multiply pixels by constants */

            res_p3_p2_p1_p0 = _mpyu4(im1_p3_p2_p1_p0, im2_p3_p2_p1_p0);
            res_p7_p6_p5_p4 = _mpyu4(im1_p7_p6_p5_p4, im2_p7_p6_p5_p4);

//               *((long long*)imgW) = r3_r2_r1_r0;

         /* store 8 pixels */

            _amem8(pRowDstu8) = _itoll(res_p7_p6_p5_p4, res_p3_p2_p1_p0);
            pRowDstu8 += 8;
         }
#endif
      }
   }
}

CV_IMPL void cvCvtYuv(const CvArr* srcarr, CvArr* dstarr, int code) {

   if (code == CV_YCrCb2BGR || code == CV_YUV2BGR) {
   
      cv::Mat dst = cv::cvarrToMat(dstarr);
      cv::cvtYuv2Mat(srcarr, dst, code);
   }
   else if (code == CV_BGR2YCrCb || code == CV_BGR2YUV) {

      cv::Mat src = cv::cvarrToMat(srcarr);
      cv::cvtMat2Yuv(src, dstarr, code);
   }    
}
