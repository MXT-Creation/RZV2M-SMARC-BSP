/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
* other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
* applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
* EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
* SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
* SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
* this software. By using this software, you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2023 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/
/***********************************************************************************************************************
* File Name    : define.h
* Version      : 7.40
* Description  : RZ/V2M DRP-AI Sample Application for Darknet-PyTorch Tiny YOLOv2 MIPI-HDMI version
***********************************************************************************************************************/

#ifndef DEFINE_MACRO_H
#define DEFINE_MACRO_H


/*****************************************
* includes
******************************************/
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/select.h>
#include <vector>
#include <map>
#include <fstream>
#include <sys/time.h>
#include <iostream>
#include <numeric>
#include <algorithm>
#include <errno.h>
#include <termios.h>
#include <math.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <iomanip>
#include <climits>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <unordered_map>
#include <pthread.h>
#include <semaphore.h>
#include <asm/param.h>


/*****************************************
* Static Variables for Tiny YOLOv2
******************************************/
/* Directory name of DRP-AI Object files (DRP-AI Translator output) */
const static std::string drpai_prefix   = "tinyyolov2_cam";
/* Pascal VOC dataset label list */
const static std::vector<std::string> label_file_map = { "aeroplane", "bicycle", "bird", "boat", "bottle", "bus", "car", "cat", "chair", "cow", "diningtable", "dog", "horse", "motorbike", "person", "pottedplant", "sheep", "sofa", "train", "tvmonitor" };

/* Number of class to be detected */
#define NUM_CLASS           (20)
/* Number for [region] layer num parameter */
#define NUM_BB              (5)
/* Number of output layers. This value MUST match with the length of num_grids[] below */
#define NUM_INF_OUT_LAYER       (1)
/* Number of grids in the image. The length of this array MUST match with the NUM_INF_OUT_LAYER */
const static uint8_t num_grids[] = { 13 };
/* Number of DRP-AI output */
const static uint32_t num_inf_out =  (NUM_CLASS + 5)* NUM_BB * num_grids[0] * num_grids[0];
/* Anchor box information */
static const double anchors[] =
{
    1.08,   1.19,
    3.42,   4.41,
    6.63,   11.38,
    9.42,   5.11,
    16.62,  10.52
};

/* Thresholds */
#define TH_PROB                 (0.4f)
#define TH_NMS                  (0.5f)
/* Size of input image to the model */
#define MODEL_IN_W              (416)
#define MODEL_IN_H              (416)

/*****************************************
* Common Macro for Application
******************************************/
/*Maximum DRP-AI Timeout threshold*/
#define DRPAI_TIMEOUT             (35000000) /* nanoseconds */

/*ISP & Inference Related Parameters*/
#define MIPI_WIDTH                (960)
#define MIPI_HEIGHT               (540)
#define MIPI_BUFFER               (8)
#define IMAGE_NUM                 (1)

/*DRP-AI Input image information*/
#define DRPAI_IN_WIDTH            (640)
#define DRPAI_IN_HEIGHT           (480)
#define DRPAI_IN_CHANNEL          (2)
#define DRPAI_IN_SIZE             (DRPAI_IN_WIDTH*DRPAI_IN_HEIGHT*DRPAI_IN_CHANNEL)
/*Pipelining Input Image*/
#define NUM_PIPELINE              (2)
#define SECOND_INPUT_ADDR         (0xCF000000)
/*For memory mapping*/
#define MEM_PAGE_SIZE             (4096)
/*HDMI Display information*/
#define AI_TIME_POS_X             (5)
#define AI_TIME_POS_Y             (5)
#define DISPLAY_WIDTH             (960)
#define DISPLAY_HEIGHT            (540)
#define DISPLAY_TEXT_SIZE         (256)
#define FONT_WIDTH                (12)
#define FONT_HEIGHT               (18)
#define BOX_MAX_DISPLAY           (5)
#define LAYER_NUM                 (1)
#define DEBUG_LAYER_NUM           (0)

/*Waiting Time*/
#define WAIT_TIME                 (1000) /* microseconds */
/*Timer Related*/
#define TIME_COEF                 (1)  /* for miliseconds */
#define DISPLAY_THREAD_TIMEOUT    (20) /* seconds */
#define AI_THREAD_TIMEOUT         (20) /* seconds */
#define KEY_THREAD_TIMEOUT        (5)  /* seconds */
#define CALLBACK_TIMEOUT          (10000) /* counts */

/*Buffer size for writing data to memory via DRP-AI Driver*/
#define BUF_SIZE                  (128*1024)

/*Index to access drpai_file_path[]*/
#define INDEX_D                   (0)
#define INDEX_C                   (1)
#define INDEX_P                   (2)
#define INDEX_A                   (3)
#define INDEX_W                   (4)

/*****************************************
* Static Variables (No need to change)
* Following variables are the file name of each DRP-AI Object file
* drpai_file_path order must be same as the INDEX_* defined later.
******************************************/
const static std::string drpai_address_file = drpai_prefix+"/"+drpai_prefix+"_addrmap_intm.txt";
const static std::string drpai_file_path[5] =
{
    drpai_prefix+"/drp_desc.bin",
    drpai_prefix+"/"+drpai_prefix+"_drpcfg.mem",
    drpai_prefix+"/drp_param.bin",
    drpai_prefix+"/aimac_desc.bin",
    drpai_prefix+"/"+drpai_prefix+"_weight.dat",
};
/*****************************************
* ERROR CODE
******************************************/
#define ERR_SYSTEM_ID             (-203)


/*****************************************
* Typedef
******************************************/
/* For DRP-AI Address List */
typedef struct {
    unsigned long desc_aimac_addr;
    unsigned long desc_aimac_size;
    unsigned long desc_drp_addr;
    unsigned long desc_drp_size;
    unsigned long drp_param_addr;
    unsigned long drp_param_size;
    unsigned long data_in_addr;
    unsigned long data_in_size;
    unsigned long data_addr;
    unsigned long data_size;
    unsigned long work_addr;
    unsigned long work_size;
    unsigned long data_out_addr;
    unsigned long data_out_size;
    unsigned long drp_config_addr;
    unsigned long drp_config_size;
    unsigned long weight_addr;
    unsigned long weight_size;
} st_addr_t;

#endif
