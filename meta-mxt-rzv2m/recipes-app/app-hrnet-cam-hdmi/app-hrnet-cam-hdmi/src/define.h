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
* Description  : RZ/V2M DRP-AI Sample Application for MMPose HRNet MIPI-HDMI version
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
* Static Variables and Macro for HRNet
******************************************/
/* Directory name of DRP-AI Object files (DRP-AI Translator output) */
const static std::string drpai_prefix   = "hrnet_cam";

/*HRNet Related*/
#define NUM_OUTPUT_W              (48)
#define NUM_OUTPUT_H              (64)
#define NUM_OUTPUT_C              (17)
/* Number of DRP-AI output */
const static uint32_t num_inf_out =  NUM_OUTPUT_W*NUM_OUTPUT_H*NUM_OUTPUT_C;
#define TH_KPT                    (0.3f)

/*****************************************
* Common Macro for Application
******************************************/
/*Maximum DRP-AI Timeout threshold*/
#define DRPAI_TIMEOUT             (35000000) /* nanoseconds */

/*CAMERA & ISP Settings Related*/
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
/*Cropping Image Related*/
#define CROPPED_IMAGE_WIDTH       (270)
#define CROPPED_IMAGE_HEIGHT      (DRPAI_IN_HEIGHT)
/*HRNet Post Processing & Drawing Related*/
#define OUTPUT_LEFT               (278)
#define OUTPUT_TOP                (0)
#define OUTPUT_WIDTH              (405)
#define OUTPUT_HEIGHT             (MIPI_HEIGHT)
#define OUTPUT_ADJ_X              (2)
#define OUTPUT_ADJ_Y              (0)
/*Graphic Drawing Settings Related*/
#define LAYER_NUM                 (1)
#define DEBUG_LAYER_NUM           (0)
#define DISPLAY_TEXT_SIZE         (256)
#define KEY_POINT_SIZE            (2)

/*Waiting Time*/
#define WAIT_TIME                 (1000) /* microseconds */
/*Timer Related*/
#define TIME_COEF                 (1)  /* for miliseconds */
#define DISPLAY_THREAD_TIMEOUT    (20) /* seconds */
#define AI_THREAD_TIMEOUT         (20) /* seconds */
#define KEY_THREAD_TIMEOUT        (5)  /* seconds */
#define CALLBACK_TIMEOUT          (10000) /* counts */
/*Inference Time Drawing Related*/
#define AI_TIME_SIZE_WIDTH        (16)
#define AI_TIME_SIZE_HEIGHT       (24)
#define AI_TIME_POS_X             (120)
#define AI_TIME_POS_Y             (5)
/*Box Message Drawing Related*/
#define MES_SIZE_WIDTH            (12)
#define MES_SIZE_HEIGHT           (16)
#define MES_POS_X                 (OUTPUT_LEFT+5)
#define MES_POS_Y                 (OUTPUT_HEIGHT-20)

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
