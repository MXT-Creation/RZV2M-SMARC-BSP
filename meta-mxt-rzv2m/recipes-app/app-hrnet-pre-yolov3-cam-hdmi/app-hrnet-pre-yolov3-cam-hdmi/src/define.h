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
* Description  : RZ/V2M DRP-AI Sample Application for MMPose HRNet with Darknet YOLOv3 MIPI HDMI version
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
#include <cstring>
#include <float.h>

/*****************************************
* Static Variables for YOLOv3
* Following variables need to be changed in order to custormize the AI model
*  - input_img = input image to DRP-AI (size and format are determined in DRP-AI Translator)
*  - drpai_prefix = directory name of DRP-AI Object files (DRP-AI Translator output)
******************************************/
const static double anchors[] =
{
    10, 13,
    16, 30,
    33, 23,
    30, 61,
    62, 45,
    59, 119,
    116, 90,
    156, 198,
    373, 326
};
const static std::string label_list = "coco-labels-2014_2017.txt";

/*****************************************
* MACROS
******************************************/
/*Buffer size for writing data to memory via DRP-AI Driver*/
#define BUF_SIZE                  (128*1024)
/*Index to access drpai_file_path[]*/
#define INDEX_D                   (0)
#define INDEX_C                   (1)
#define INDEX_P                   (2)
#define INDEX_A                   (3)
#define INDEX_W                   (4)
#define NUM_OUT_LAYER 3
/* Number of class to be detected */
#define NUM_CLASS                 (80)
/* Number of grids in the image */
#define NUM_GRID_1 13
#define NUM_GRID_2 26
#define NUM_GRID_3 52

/*Maximum DRP-AI Timeout threshold*/
#define DRPAI_TIMEOUT             (35000000) /* nanoseconds */
/*Waiting Time*/
#define WAIT_TIME                 (1000)
/*CAMERA & ISP Settings Related*/
#define MIPI_WIDTH                (960)
#define MIPI_HEIGHT               (540)
#define MIPI_BUFFER               (8)
#define IMAGE_NUM                 (1)
#define IMREAD_IMAGE_WIDTH        (640)
#define IMREAD_IMAGE_HEIGHT       (480)
#define IMREAD_IMAGE_CHANNEL      (2)
#define IMREAD_IMAGE_SIZE         (IMREAD_IMAGE_WIDTH*IMREAD_IMAGE_HEIGHT*IMREAD_IMAGE_CHANNEL)
/*Pipelining Input Image*/
#define NUM_PIPELINE              (2)
#define SECOND_INPUT_ADDR         (0xDF000000)
/*Inference Related Parameters*/
#define AI0_DESC_NAME             "yolov3_cam"
#define AI_DESC_NAME              "hrnet_cam"
/*TinyYOLOv2 related*/
/* Number of class to be detected */
#define NUM_CLASS                 (80)
/* Number of grids in the image */
#define NUM_GRID_X                (13)
#define NUM_GRID_Y                (13)
/* Number for [region] layer num parameter */
#define NUM_BB                    (3)
/* Thresholds */
#define TH_PROB                   (0.6f)
#define TH_NMS                    (0.5f)
/* Size of input image to the model */
#define MODEL_IN_W                (416)
#define MODEL_IN_H                (416)
#define DISPLAY_WIDTH             (MIPI_WIDTH)
#define DISPLAY_HEIGHT            (MIPI_HEIGHT)
#define DISPLAY_TEXT_SIZE         (256)
#define INF_OUT_SIZE_YOLO         (0x373c8c)
/*HRNet Related*/
#define NUM_OUTPUT_W              (48)
#define NUM_OUTPUT_H              (64)
#define NUM_OUTPUT_C              (17)
#define INF_OUT_SIZE              (NUM_OUTPUT_W*NUM_OUTPUT_H*NUM_OUTPUT_C)
/*HRNet Post Processing & Drawing Related*/
#define TH_KPT                    (0.10f)
#define OUTPUT_ADJ_X              (2)
#define OUTPUT_ADJ_Y              (0)
#define NUM_MAX_PERSON            (7)
#define CROP_ADJ_X                (20)
#define CROP_ADJ_Y                (20)
/*Graphic Drawing Settings Related*/
#define LAYER_NUM                 (1)
#define DEBUG_LAYER_NUM           (0)
#define KEY_POINT_SIZE            (2)
/*Inference Time Drawing Related*/
#define AI_TIME_SIZE_WIDTH        (16)
#define AI_TIME_SIZE_HEIGHT       (24)
#define AI_TIME_POS_X             (120)
#define AI_TIME_POS_Y             (5)
/*Timer Related*/
#define TIME_COEF                 (1)
#define AI_THREAD_TIMEOUT         (20)
#define KEY_THREAD_TIMEOUT        (5)
#define LOCK_TIME                 (1)
#define CALLBACK_TIMEOUT          (10000)
/*For memory mapping*/
#define MEM_PAGE_SIZE             (4096)
#define PERSON_LABEL_NUM          (0)

/*****************************************
* ERROR CODE
******************************************/
#define SUCCESS                   (0x00)
#define ERR_ADDRESS_CMA           (0x01)
#define ERR_OPEN                  (0x10)
#define ERR_FORMAT                (0x11)
#define ERR_READ                  (0x12)
#define ERR_MMAP                  (0x13)
#define ERR_MLOCK                 (0x14)
#define ERR_MALLOC                (0x15)
#define ERR_GET_TIME              (0x16)
#define ERR_DRPAI_TIMEOUT         (0x20)
#define ERR_DRPAI_START           (0x21)
#define ERR_DRPAI_ASSIGN          (0x22)
#define ERR_DRPAI_WRITE           (0x23)
#define ERR_DRPAI_ASSIGN_PARAM    (0x24)
#define ERR_SYSTEM_ID             (-203)


/*****************************************
* Typedef
******************************************/
/* For DRP-AI Address List */
typedef struct {
    uint64_t desc_aimac_addr;
    uint64_t desc_aimac_size;
    uint64_t desc_drp_addr;
    uint64_t desc_drp_size;
    uint64_t drp_param_addr;
    uint64_t drp_param_size;
    uint64_t data_in_addr;
    uint64_t data_in_size;
    uint64_t data_addr;
    uint64_t data_size;
    uint64_t work_addr;
    uint64_t work_size;
    uint64_t data_out_addr;
    uint64_t data_out_size;
    uint64_t drp_config_addr;
    uint64_t drp_config_size;
    uint64_t weight_addr;
    uint64_t weight_size;
} addr;

#endif
