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
* File Name    : sample_app_tinyyolov2_cam_hdmi.cpp
* Version      : 7.40
* Description  : RZ/V2M DRP-AI Sample Application for Darknet-PyTorch Tiny YOLOv2 MIPI-HDMI version
***********************************************************************************************************************/

/*****************************************
* Includes
******************************************/
/*DRPAI Driver Header*/
#include <linux/drpai.h>
/*Definition of Macros & other variables*/
#include "define.h"
/*Bounding Box for Tiny Yolo V2 Post Processing*/
#include "box.h"
/*Core 1 Command Controller Header*/
#include "comctl/comctl_if.h"

/*****************************************
* Global Variables
******************************************/
/*Multithreading*/
static sem_t terminate_req_sem;
static pthread_t ai_inf_thread;
static pthread_t kbhit_thread;
static pthread_t hdmi_thread;
/*Flags*/
static uint32_t image_frame_ready = 0;
static uint32_t image_frame_used = 0;
static uint32_t image_show_ready = 0;
static uint32_t image_show_used = 0;
volatile static atomic_flag image_frame_show_grd = ATOMIC_FLAG_INIT;
/*Global Variables*/
static uint8_t drpai_buf[BUF_SIZE];
static float drpai_output_buf[num_inf_out];
static int64_t source_id = 0;
static st_video_frame_info_t frame_info;
static st_image_info_t image_info_disp;
static st_image_info_t image_info;
static st_image_object_t image_obj_disp;
static st_image_object_t image_obj_disp_cp;
static st_image_object_t image_obj;
static uint64_t locked_handle;
static uint64_t locked_handle_hdmi;
static st_display_info_t disp_info;
static drpai_data_t drpai_data_in[NUM_PIPELINE];
static int8_t idata_idx = 0; // Indicates the used input data.
/*AI Inference for DRPAI*/
static uint32_t bcount = 0;
static std::vector<detection> det_res;
static int16_t x_center = 0;
static int16_t y_center = 0;
static int16_t x_left = 0;
static int16_t y_top = 0;
static int16_t w_rect = 0;
static int16_t h_rect = 0;
static st_addr_t drpai_address;
static int8_t drpai_fd = -1;
static uint32_t ai_time;
static char ai_time_str[DISPLAY_TEXT_SIZE];
static char inf_res_str[DISPLAY_TEXT_SIZE];

/*****************************************
* Function Name : wait_join
* Description   : waits for a fixed amount of time for the thread to exit
* Arguments     : p_join_thread = thread that the function waits for to Exit
*                 join_time = the timeout time for the thread for exiting
* Return value  : 0 if successful
*                 not 0 otherwise
******************************************/
static int8_t wait_join(pthread_t *p_join_thread, uint32_t join_time)
{
    int8_t ret_err;
    struct timespec join_timeout;
    ret_err = clock_gettime(CLOCK_REALTIME, &join_timeout);
    if ( 0 == ret_err )
    {
        join_timeout.tv_sec += join_time;
        ret_err = pthread_timedjoin_np(*p_join_thread, NULL, &join_timeout);
    }
    return ret_err;
}

/*****************************************
* Function Name : image_frame_callback
* Description   : function executed when Capture Image is ready
*                 raises the image_frame_ready flag
* Arguments     : error = returned value from B_IMAGE_StartVideoCapture callback
* Return value  : -
******************************************/
void image_frame_callback(err_t error)
{
    /*Set Atomic Flag Value To 1*/
    while (atomic_flag_test_and_set(&image_frame_show_grd));
    /*Set Image Frame Ready & Image Show Ready Flag*/
    image_frame_ready++;
    image_show_ready++;
    /*Reset Atomic Flag Value To 0*/
    atomic_flag_clear(&image_frame_show_grd);

    return;
}

/*****************************************
* Function Name : sigmoid
* Description   : Helper function for YOLO Post Processing
* Arguments     : x = input argument for the calculation
* Return value  : sigmoid result of input x
******************************************/
double sigmoid(double x)
{
    return 1.0/(1.0+exp(-x));
}

/*****************************************
* Function Name : softmax
* Description   : Helper function for YOLO Post Processing
* Arguments     : val[] = array to be computed softmax
* Return value  : -
******************************************/
void softmax(float val[NUM_CLASS])
{
    float max_num = -INT_MAX;
    float sum = 0;
    int32_t i = 0;
    for ( i = 0 ; i<NUM_CLASS ; i++ )
    {
        max_num = std::max(max_num, val[i]);
    }

    for ( i = 0 ; i<NUM_CLASS ; i++ )
    {
        val[i]= (float) exp(val[i] - max_num);
        sum+= val[i];
    }

    for ( i = 0 ; i<NUM_CLASS ; i++ )
    {
        val[i]= val[i]/sum;
    }
    return;
}

/*****************************************
* Function Name : yolo_index
* Description   : Get the index of the bounding box attributes based on the input offset.
* Arguments     : n = output layer number.
*                 offs = offset to access the bounding box attributesd.
*                 channel = channel to access each bounding box attribute.
* Return value  : index to access the bounding box attribute.
******************************************/
int32_t yolo_index(uint8_t n, int32_t offs, int32_t channel)
{
    uint8_t num_grid = num_grids[n];
    return offs + channel * num_grid * num_grid;
}

/*****************************************
* Function Name : yolo_offset
* Description   : Get the offset nuber to access the bounding box attributes
*                 To get the actual value of bounding box attributes, use yolo_index() after this function.
* Arguments     : n = output layer number.
*                 b = Number to indicate which bounding box in the region.
*                 y = Number to indicate which region [0~13]
*                 x = Number to indicate which region [0~13]
* Return value  : offset to access the bounding box attributes.
******************************************/
int32_t yolo_offset(uint8_t n, int32_t b, int32_t y, int32_t x)
{
    uint8_t num = num_grids[n];
    uint32_t prev_layer_num = 0;
    int32_t i = 0;

    for (i = 0 ; i < n; i++)
    {
        prev_layer_num += NUM_BB *(NUM_CLASS + 5)* num_grids[i] * num_grids[i];
    }
    return prev_layer_num + b *(NUM_CLASS + 5)* num * num + y * num + x;
}

/*****************************************
* Function Name : print_boxes
* Description   : Function to printout details of detected bounding boxes to standard output
* Arguments     : det = detected boxes details
*                 box_count = total number of boxes
* Return value  : -
******************************************/
void print_boxes(std::vector<detection>& det, uint32_t box_count)
{
    uint32_t real_count = 0;
    float x, y, w, h;
    uint32_t i = 0;
    for (i = 0; i<box_count; i++)
    {
        if (det[i].prob == 0)
        {
            continue;
        }
        else
        {
            real_count++;
            x = det[i].bbox.x;
            y = det[i].bbox.y;
            w = det[i].bbox.w;
            h = det[i].bbox.h;
            printf(" Bounding Box Number: %d\n", real_count);
            printf(" Bounding Box       : (X, Y, W, H) = (%.0f, %.0f, %.0f, %.0f)\n", x, y, w, h);
            printf("\x1b[32;1m Detected  Class    : %s (%d)\n\x1b[0m",label_file_map[det[i].c].c_str(), det[i].c);
            printf("  Probability       : %5.1f %%\n", det[i].prob*100);
        }
    }
    printf(" Bounding Box Count   : %d\n", real_count);
}

/*****************************************
* Function Name : R_Post_Proc
* Description   : CPU post-processing for Tiny YOLOv2
* Arguments     : floatarr = drpai output data pointer
*                 det = detected boxes details
*                 box_count = total number of boxes
* Return value  : -
******************************************/
void R_Post_Proc(float* floatarr, std::vector<detection>& det, uint32_t* box_count)
{
    uint32_t count = 0;
    /* Following variables are required for correct_region_boxes in Darknet implementation*/
    /* Note: This implementation refers to the "darknet detector test" */
    float new_w, new_h;
    float correct_w = 1.;
    float correct_h = 1.;
    if ((float) (MODEL_IN_W / correct_w) < (float) (MODEL_IN_H/correct_h) )
    {
        new_w = (float) MODEL_IN_W;
        new_h = correct_h * MODEL_IN_W / correct_w;
    }
    else
    {
        new_w = correct_w * MODEL_IN_H / correct_h;
        new_h = MODEL_IN_H;
    }

    uint32_t n = 0;
    uint32_t b = 0;
    uint32_t y = 0;
    uint32_t x = 0;
    uint32_t offs = 0;
    int32_t i = 0;
    float tx = 0;
    float ty = 0;
    float tw = 0;
    float th = 0;
    double tc = 0;
    float center_x = 0;
    float center_y = 0;
    float box_w = 0;
    float box_h = 0;
    double objectness = 0;
    uint8_t num_grid = 0;
    uint8_t anchor_offset = 0;
    float classes[NUM_CLASS];
    float max_pred = 0;
    int8_t pred_class = -1;
    float probability = 0;
    detection d;
    Box bb;
    /*Post Processing Start*/
    for (n = 0; n<NUM_INF_OUT_LAYER; n++)
    {
        num_grid = num_grids[n];
        anchor_offset = 2 * NUM_BB * (NUM_INF_OUT_LAYER - (n + 1));

        for (b = 0;b<NUM_BB;b++)
        {
            for (y = 0;y<num_grid;y++)
            {
                for (x = 0;x<num_grid;x++)
                {
                    offs = yolo_offset(n, b, y, x);
                    tx = floatarr[offs];
                    ty = floatarr[yolo_index(n, offs, 1)];
                    tw = floatarr[yolo_index(n, offs, 2)];
                    th = floatarr[yolo_index(n, offs, 3)];
                    tc = floatarr[yolo_index(n, offs, 4)];
                    /* Compute the bounding box */
                    /*get_region_box*/
                    center_x = ((float) x + sigmoid(tx)) / (float) num_grid;
                    center_y = ((float) y + sigmoid(ty)) / (float) num_grid;
                    box_w = (float) exp(tw) * anchors[anchor_offset+2*b+0] / (float) num_grid;
                    box_h = (float) exp(th) * anchors[anchor_offset+2*b+1] / (float) num_grid;
                    /* Size Adjustment*/
                    /* correct_region_boxes */
                    center_x = (center_x - (MODEL_IN_W - new_w) / 2. / MODEL_IN_W) / ((float) new_w / MODEL_IN_W);
                    center_y = (center_y - (MODEL_IN_H - new_h) / 2. / MODEL_IN_H) / ((float) new_h / MODEL_IN_H);
                    box_w *= (float) (MODEL_IN_W / new_w);
                    box_h *= (float) (MODEL_IN_H / new_h);

                    center_x = round(center_x * DISPLAY_WIDTH);
                    center_y = round(center_y * DISPLAY_HEIGHT);
                    box_w = round(box_w * DISPLAY_WIDTH);
                    box_h = round(box_h * DISPLAY_HEIGHT);

                    objectness = sigmoid(tc);

                    bb = {center_x, center_y, box_w, box_h};
                    /* Get the class prediction */
                    for (i = 0; i < NUM_CLASS; i++)
                    {
                        classes[i] = floatarr[yolo_index(n, offs, 5+i)];
                    }
                    softmax(classes);
                    max_pred = 0;
                    pred_class = -1;
                    for (i = 0; i < NUM_CLASS; i++)
                    {
                        if (classes[i] > max_pred)
                        {
                            pred_class = i;
                            max_pred = classes[i];
                        }
                    }

                    /* Store the result into the list if the probability is more than the threshold */
                    probability = max_pred * objectness;
                    if (probability > TH_PROB)
                    {
                        d = {bb, pred_class, probability};
                        det.push_back(d);
                        count++;
                    }
                }
            }
        }
    }
    /* Non-Maximum Supression filter */
    filter_boxes_nms(det, count, TH_NMS);
    *box_count = count;
}

/*****************************************
* Function Name : timedifference_msec
* Description   : computes the time difference in ms between two moments
* Arguments     : t0 = start time
*                 t1 = stop time
* Return value  : the time difference in ms
******************************************/
static double timedifference_msec(struct timespec t0, struct timespec t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1000000.0;
}

/*****************************************
* Function Name : read_addrmap_txt
* Description   : Loads address and size of DRP-AI Object files into struct addr.
* Arguments     : addr_file = filename of addressmap file (from DRP-AI Object files)
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
int8_t read_addrmap_txt(std::string addr_file)
{
    std::string str;
    uint32_t l_addr;
    uint32_t l_size;
    std::string element, a, s;

    std::ifstream ifs(addr_file);
    if (ifs.fail())
    {
        fprintf(stderr, "[ERROR] Failed to open address map list : %s\n", addr_file.c_str());
        return -1;
    }

    while (getline(ifs, str))
    {
        std::istringstream iss(str);
        iss >> element >> a >> s;
        l_addr = strtol(a.c_str(), NULL, 16);
        l_size = strtol(s.c_str(), NULL, 16);

        if ("drp_config" == element)
        {
            drpai_address.drp_config_addr = l_addr;
            drpai_address.drp_config_size = l_size;
        }
        else if ("desc_aimac" == element)
        {
            drpai_address.desc_aimac_addr = l_addr;
            drpai_address.desc_aimac_size = l_size;
        }
        else if ("desc_drp" == element)
        {
            drpai_address.desc_drp_addr = l_addr;
            drpai_address.desc_drp_size = l_size;
        }
        else if ("drp_param" == element)
        {
            drpai_address.drp_param_addr = l_addr;
            drpai_address.drp_param_size = l_size;
        }
        else if ("weight" == element)
        {
            drpai_address.weight_addr = l_addr;
            drpai_address.weight_size = l_size;
        }
        else if ("data_in" == element)
        {
            drpai_address.data_in_addr = l_addr;
            drpai_address.data_in_size = l_size;
        }
        else if ("data" == element)
        {
            drpai_address.data_addr = l_addr;
            drpai_address.data_size = l_size;
        }
        else if ("data_out" == element)
        {
            drpai_address.data_out_addr = l_addr;
            drpai_address.data_out_size = l_size;
        }
        else if ("work" == element)
        {
            drpai_address.work_addr = l_addr;
            drpai_address.work_size = l_size;
        }
        else
        {
            /*Ignore other space*/
        }
    }

    return 0;
}

/*****************************************
* Function Name : load_data_to_mem
* Description   : Loads a file to memory via DRP-AI Driver
* Arguments     : data = filename to be written to memory
*                 drpai_fd = file descriptor of DRP-AI Driver
*                 from = memory start address where the data is written
*                 size = data size to be written
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
int8_t load_data_to_mem(std::string data, int8_t drpai_fd, uint32_t from, uint32_t size)
{
    int8_t ret_load_data = 0;
    int8_t obj_fd;
    drpai_data_t drpai_data;
    size_t ret = 0;
    int32_t i = 0;

    printf("[START] Loading %s: size %X at address %X\n", data.c_str(), size, from);
    errno = 0;
    obj_fd = open(data.c_str(), O_RDONLY);
    if (0 > obj_fd)
    {
        fprintf(stderr, "[ERROR] Failed to open: %s errno=%d\n", data.c_str(), errno);
        ret_load_data = -1;
        goto end;
    }

    drpai_data.address = from;
    drpai_data.size = size;

    errno = 0;
    ret = ioctl(drpai_fd, DRPAI_ASSIGN, &drpai_data);
    if (0 != ret)
    {
        fprintf(stderr, "[ERROR] Failed to run DRPAI_ASSIGN: errno=%d\n", errno);
        ret_load_data = -1;
        goto end;
    }

    for (i = 0 ; i<(drpai_data.size/BUF_SIZE) ; i++)
    {
        errno = 0;
        ret = read(obj_fd, drpai_buf, BUF_SIZE);
        if (0 > ret)
        {
            fprintf(stderr, "[ERROR] Failed to read: %s errno=%d\n", data.c_str(), errno);
            ret_load_data = -1;
            goto end;
        }
        ret = write(drpai_fd, drpai_buf, BUF_SIZE);
        if (0 > ret)
        {
            fprintf(stderr, "[ERROR] Failed to write via DRP-AI Driver: errno=%d\n", errno);
            ret_load_data = -1;
            goto end;
        }
    }
    if ( 0 != (drpai_data.size % BUF_SIZE))
    {
        errno = 0;
        ret = read(obj_fd, drpai_buf, (drpai_data.size % BUF_SIZE));
        if ( 0 > ret )
        {
            fprintf(stderr, "[ERROR] Failed to read: %s errno=%d\n", data.c_str(), errno);
            ret_load_data = -1;
            goto end;
        }
        ret = write(drpai_fd, drpai_buf, (drpai_data.size % BUF_SIZE));
        if (0 > ret)
        {
            fprintf(stderr, "[ERROR] Failed to write via DRP-AI Driver: errno=%d\n", errno);
            ret_load_data = -1;
            goto end;
        }
    }
    goto end;

end:
    if (0 <= obj_fd)
    {
        close(obj_fd);
    }
    if (0 == ret_load_data)
    {
        printf("[END] Loading  %s\n", data.c_str());
    }
    return ret_load_data;
}

/*****************************************
* Function Name : load_drpai_data
* Description   : Loads DRP-AI Object files to memory via DRP-AI Driver.
* Arguments     : drpai_fd = file descriptor of DRP-AI Driver
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
int8_t load_drpai_data(int8_t drpai_fd)
{
    uint32_t addr = 0;
    uint32_t size = 0;
    uint8_t i = 0;
    int8_t ret = 0;
    double diff = 0;

    struct timespec start_time, stop_time;
    /*Start load time*/
    ret = timespec_get(&start_time, TIME_UTC);
    if (0 == ret)
    {
        fprintf(stderr,"[ERROR] Failed to run timespect_get().\n");
        return -1;
    }
    printf("[START] Loading DRP-AI Data...\n");
    for (i = 0; i < 5; i++)
    {
        switch (i)
        {
            case (INDEX_W):
                addr = drpai_address.weight_addr;
                size = drpai_address.weight_size;
                break;
            case (INDEX_C):
                addr = drpai_address.drp_config_addr;
                size = drpai_address.drp_config_size;
                break;
            case (INDEX_P):
                addr = drpai_address.drp_param_addr;
                size = drpai_address.drp_param_size;
                break;
            case (INDEX_A):
                addr = drpai_address.desc_aimac_addr;
                size = drpai_address.desc_aimac_size;
                break;
            case (INDEX_D):
                addr = drpai_address.desc_drp_addr;
                size = drpai_address.desc_drp_size;
                break;
            default:
                break;
        }

        ret = load_data_to_mem(drpai_file_path[i], drpai_fd, addr, size);
        if (0 > ret)
        {
            fprintf(stderr,"[ERROR] Failed to load data from memory: %s\n",drpai_file_path[i].c_str());
            return -1;
        }
    }

    /*Stop load time*/
    ret = timespec_get(&stop_time, TIME_UTC);
    if (0 == ret)
    {
        fprintf(stderr,"[ERROR] Failed to run timespect_get().\n");
        return -1;
    }
    diff = timedifference_msec(start_time,stop_time);
    printf("[END] Loading DRP-AI Data : Total loading time %f s\n", diff*0.001);
    return 0;
}

/*****************************************
* Function Name : get_input
* Description   : Get DRP-AI Input from memory via DRP-AI Driver
* Arguments     : mem_fd = file descriptor of memory device file
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
int8_t get_input(int mem_fd)
{
    int8_t ret = 0;
    int8_t ret_drpai = 0;
    /*Core1 Command Controller Variables*/
    bool r_image_get = false;
    err_t inf_isp_err = 0;
    uint8_t *image = NULL;
    /*For Image Copy*/
    uint8_t* drpai_img_map = (uint8_t*)MAP_FAILED;
    uint32_t drpai_img_size = DRPAI_IN_SIZE;
    uint32_t drpai_img_addr;
    uint32_t drpai_img_start_addr;
    uint32_t drpai_img_offset;

    /*Get Image Object For AI Inference*/
    inf_isp_err = B_IMAGE_Imread(source_id,&image_info,IMAGE_NUM,&image_obj);
    if (0 != inf_isp_err)
    {
        fprintf(stderr, "[ERROR] Failed to run B_IMAGE_Imread: %d\n", inf_isp_err);
        goto err;
    }
    /*Gets Memory Address Of Image Stored & Lock Image Object*/
    image = (uint8_t*)B_OBJ_BeginReadingData(image_obj.handle);
    if (NULL == image)
    {
        fprintf(stderr, "[ERROR] Failed to run B_OBJ_BeginReadingData\n");
        goto err;
    }
    else
    {
        /*Save Locked Handle*/
        locked_handle = image_obj.handle;
        /*When Successful Read*/
        r_image_get = true;
    }
    /*Gets the Image Virtual Address*/
    drpai_img_addr = (off_t)(image); //image physical address
    drpai_img_offset = drpai_img_addr%(MEM_PAGE_SIZE);
    drpai_img_start_addr = drpai_img_addr-drpai_img_offset;
    drpai_img_map = (uint8_t*)(mmap(NULL,(size_t)(drpai_img_size+drpai_img_offset),PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd,drpai_img_start_addr));
    if (MAP_FAILED == drpai_img_map)
    {
        fprintf(stderr, "[ERROR] Failed to mmap /dev/mem for Captured Image Frame\n");
        goto err;
    }
    /*write copy image to drpai*/
    errno = 0;
    ret_drpai = ioctl(drpai_fd, DRPAI_ASSIGN, &drpai_data_in[idata_idx]);
    if (0 != ret_drpai)
    {
        fprintf(stderr, "[ERROR] Failed to assign DRPAI Input data: %d\n", errno);
        goto err;
    }
    ret_drpai = write(drpai_fd, &drpai_img_map[drpai_img_offset], drpai_data_in[idata_idx].size);
    if (0 > ret_drpai)
    {
        fprintf(stderr, "[ERROR] Failed to write DRPAI Input data: %d\n", errno);
        goto err;
    }
    /*unmap the Image Virtual Address*/
    munmap(drpai_img_map,(size_t)(drpai_img_size+drpai_img_offset));
    drpai_img_map = (uint8_t*)MAP_FAILED;
    /*Release Image Object*/
    inf_isp_err = B_OBJ_EndReadingData(locked_handle);
    if (0 != inf_isp_err)
    {
        fprintf(stderr, "[ERROR] Failed to run B_OBJ_EndReadingData: %d\n", inf_isp_err);
        goto err;
    }
    else
    {
        r_image_get = false;
    }
    goto end;

err:
    ret = -1;
    goto end;
end:
    if (drpai_img_map != MAP_FAILED)
    {
        munmap(drpai_img_map,(size_t)(drpai_img_size+drpai_img_offset));
    }
    /*Release Image Object*/
    if (r_image_get)
    {
        B_OBJ_EndReadingData(image_obj.handle);
        r_image_get = false;
    }
    return ret;
}

/*****************************************
* Function Name : R_Inf_Thread
* Description   : Executes the DRP-AI inference thread
* Arguments     : threadid = thread identification
* Return value  : -
******************************************/
void *R_Inf_Thread(void *threadid)
{
    /*Semaphore Variable*/
    int32_t inf_sem_check = 0;
    /*Core1 Command Controller Variables*/
    err_t inf_isp_err = 0;
    bool r_image_get = false;
    uint8_t *image = NULL;
    /*Variable for getting Inference output data*/
    drpai_data_t drpai_data;
    /*Inference Variables*/
    fd_set rfds;
    struct timespec tv;
    int8_t inf_status = 0;
    drpai_data_t proc[DRPAI_INDEX_NUM];
    int32_t inf_cnt = -1;
    drpai_status_t drpai_status;
    int32_t idata_swap_flag = 0;
    sigset_t sigset;
    /*Variable for checking return value*/
    int8_t ret = 0;
    int8_t ret_drpai = 0;
    /*For Image Copy*/
    int8_t drpai_mem_fd = -1;
    /*Variable for Performance Measurement*/
    static struct timespec start_time;
    static struct timespec inf_end_time;
    /*Callback Timeout Counter*/
    uint32_t cb_time_cnt = 0;
    /*Setting SIGUSR1 Signal*/
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);

    printf("Inference Thread Starting\n");

    proc[DRPAI_INDEX_INPUT].address = drpai_address.data_in_addr;
    proc[DRPAI_INDEX_INPUT].size = drpai_address.data_in_size;
    proc[DRPAI_INDEX_DRP_CFG].address = drpai_address.drp_config_addr;
    proc[DRPAI_INDEX_DRP_CFG].size = drpai_address.drp_config_size;
    proc[DRPAI_INDEX_DRP_PARAM].address = drpai_address.drp_param_addr;
    proc[DRPAI_INDEX_DRP_PARAM].size = drpai_address.drp_param_size;
    proc[DRPAI_INDEX_AIMAC_DESC].address = drpai_address.desc_aimac_addr;
    proc[DRPAI_INDEX_AIMAC_DESC].size = drpai_address.desc_aimac_size;
    proc[DRPAI_INDEX_DRP_DESC].address = drpai_address.desc_drp_addr;
    proc[DRPAI_INDEX_DRP_DESC].size = drpai_address.desc_drp_size;
    proc[DRPAI_INDEX_WEIGHT].address = drpai_address.weight_addr;
    proc[DRPAI_INDEX_WEIGHT].size = drpai_address.weight_size;
    proc[DRPAI_INDEX_OUTPUT].address = drpai_address.data_out_addr;
    proc[DRPAI_INDEX_OUTPUT].size = drpai_address.data_out_size;

    /*DRP-AI Output Memory Preparation*/
    drpai_data.address = drpai_address.data_out_addr;
    drpai_data.size = drpai_address.data_out_size;
    /*DRP-AI Input Memory Preparation*/
    drpai_data_in[0].address = drpai_address.data_in_addr;
    drpai_data_in[0].size = drpai_address.data_in_size;
    drpai_data_in[1].address = SECOND_INPUT_ADDR;
    drpai_data_in[1].size = drpai_address.data_in_size;
    /*Set Image Info For AI Inference*/
    image_info.width  = DRPAI_IN_WIDTH;
    image_info.height = DRPAI_IN_HEIGHT;
    image_info.format = D_IMAGE_FORMAT_YUYV;
    /*Open Memory Access*/
    drpai_mem_fd = open("/dev/mem", O_RDWR);
    if (0 >= drpai_mem_fd)
    {
        fprintf(stderr, "[ERROR]  Failed to open /dev/mem for Image\n");
        goto err;
    }

    printf("Inference Loop Starting\n");
    /*Inference Loop Start*/
    while (1)
    {
        if (0 == idata_swap_flag)
        {
            /*Image Ready Flag Monitor*/
            while (1)
            {
                /*Gets the Termination request semaphore value. If different then 1 Termination was requested*/
                /*Checks if sem_getvalue is executed wihtout issue*/
                errno = 0;
                ret = sem_getvalue(&terminate_req_sem, &inf_sem_check);
                if (0 != ret)
                {
                    fprintf(stderr, "[ERROR] Failed to get Semaphore Value: errno=%d\n", errno);
                    goto err;
                }
                /*Checks the semaphore value*/
                if (1 != inf_sem_check)
                {
                    goto ai_inf_end;
                }
                /*Check if image frame is ready*/
                if (image_frame_ready == image_frame_used)
                {
                    usleep(WAIT_TIME); //wait 1 tick time
                    /*Callback Timeout Counter Up*/
                    cb_time_cnt++;
                    if (CALLBACK_TIMEOUT <= cb_time_cnt)
                    {
                        fprintf(stderr, "[ERROR] B_IMAGE_StartVideoCapture Callback Timeout\n");
                        goto err;
                    }
                }
                else
                {
                    image_frame_used = image_frame_ready;
                    break;
                }
            }
            /*Swap input buffer*/
            idata_idx = !idata_idx;
            idata_swap_flag++;
            /*Reset callback timeout counter*/
            cb_time_cnt = 0;
            /*Copy input image to expanded area*/
            ret = get_input(drpai_mem_fd);
            if (0 != ret)
            {
                fprintf(stderr, "[ERROR] Failed to get DRPAI input data\n");
                goto err;
            }
        }
        /*Writes Data Input Physical Address & Size Directly to DRP-AI*/
        proc[DRPAI_INDEX_INPUT] = drpai_data_in[idata_idx];
        /*Gets AI Inference starting time*/
        ret = timespec_get(&start_time, TIME_UTC);
        if (0 == ret)
        {
            fprintf(stderr, "[ERROR] Failed to get Inference Start Time\n");
            goto err;
        }
        /*Start DRP-AI Driver*/
        errno = 0;
        ret = ioctl(drpai_fd, DRPAI_START, &proc[0]);
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to run DRPAI_START: errno=%d\n", errno);
            goto err;
        }
        inf_cnt++;
        /*Reset swap flag*/
        idata_swap_flag = 0;
        if (image_frame_ready != image_frame_used)
        {
            image_frame_used = image_frame_ready;
            /*Swap input buffer*/
            if (0 == idata_swap_flag)
            {
                idata_idx = !idata_idx;
                idata_swap_flag++;
            }
            /*Copy input image to expanded area*/
            ret = get_input(drpai_mem_fd);
            if (0 != ret)
            {
                fprintf(stderr, "[ERROR] Failed to get DRPAI input data\n");
                goto err;
            }
        }
        printf("[START] Start DRP-AI inference...\n");
        printf("Inference ----------- No. %d\n",(inf_cnt+1));
        while(1)
        {
            /*Gets the Termination request semaphore value. If different then 1 Termination was requested*/
            /*Checks if sem_getvalue is executed wihtout issue*/
            errno = 0;
            ret = sem_getvalue(&terminate_req_sem, &inf_sem_check);
            if (0 != ret)
            {
                fprintf(stderr, "[ERROR] Failed to get Semaphore Value: errno=%d\n", errno);
                goto err;
            }
            /*Checks the semaphore value*/
            if (1 != inf_sem_check)
            {
                goto ai_inf_end;
            }
            /*Setup pselect settings*/
            FD_ZERO(&rfds);
            FD_SET(drpai_fd, &rfds);
            tv.tv_sec = 0;
            tv.tv_nsec = DRPAI_TIMEOUT;
            /*Wait Till The DRP-AI Ends*/
            ret_drpai = pselect(drpai_fd+1, &rfds, NULL, NULL, &tv, &sigset);
            if (0 == ret_drpai)
            {
                if (image_frame_ready != image_frame_used)
                {
                    image_frame_used = image_frame_ready;
                    /*Swap input buffer*/
                    if (0 == idata_swap_flag)
                    {
                        idata_idx = !idata_idx;
                        idata_swap_flag++;
                    }
                    /*Copy input image to expanded area*/
                    ret = get_input(drpai_mem_fd);
                    if (0 != ret)
                    {
                        fprintf(stderr, "[ERROR] Failed to get DRPAI input data\n");
                        goto err;
                    }
                }
            }
            else if (0 > ret_drpai)
            {
                fprintf(stderr, "[ERROR] DRP-AI Inference pselect() Error: errno=%d\n", errno);
                goto err;
            }
            else
            {
                /*Gets AI Inference End Time*/
                ret = timespec_get(&inf_end_time, TIME_UTC);
                if (0 == ret)
                {
                    fprintf(stderr, "[ERROR] Failed to Get Inference End Time\n");
                    goto err;
                }
                /*Checks If DRPAI Inference Ended Without Issue*/
                inf_status = ioctl(drpai_fd, DRPAI_GET_STATUS, &drpai_status);
                if (0 == inf_status)
                {
                    /*Get DRP-AI Output Data*/
                    errno = 0;
                    ret = ioctl(drpai_fd, DRPAI_ASSIGN, &drpai_data);
                    if (0 != ret)
                    {
                        fprintf(stderr, "[ERROR] Failed to run DRPAI_ASSIGN: errno=%d\n", errno);
                        goto err;
                    }
                    ret = read(drpai_fd, drpai_output_buf, drpai_data.size);
                    if ( 0 > ret )
                    {
                        fprintf(stderr, "[ERROR] Failed to read via DRP-AI Driver: errno=%d\n", errno);
                        goto err;
                    }
                    /*Preparation for Post-Processing*/
                    bcount = 0;
                    det_res.clear();
                    /*CPU Post-Processing for Tiny YOLOv2*/
                    R_Post_Proc(drpai_output_buf, det_res, &bcount);
                    /*Displays AI Inference results & Processing Time on console*/
                    print_boxes(det_res, bcount);
                    /*Display Processing Time On Console*/
                    ai_time = (uint32_t)((timedifference_msec(start_time, inf_end_time)*TIME_COEF));
                    printf("AI Inference Time %d [ms]\n", ai_time);
                    break;
                }
                else //inf_status != 0
                {
                    fprintf(stderr, "[ERROR] DRPAI Internal Error: errno=%d\n", errno);
                    goto err;
                }
            }
        }
    }
    /*End of Inference Loop*/

/*Error Processing*/
err:
    /*Set Termination Request Semaphore to 0*/
    sem_trywait(&terminate_req_sem);
    goto ai_inf_end;
/*AI Thread Termination*/
ai_inf_end:
    if (drpai_mem_fd >= 0)
    {
        if (close(drpai_mem_fd) != 0)
        {
            fprintf(stderr, "[ERROR] Failed to Close /dev/mem: %d\n", errno);
        }
    }
    /*Release Image Object*/
    if (r_image_get)
    {
        B_OBJ_EndReadingData(image_obj.handle);
        r_image_get = false;
    }
    printf("AI Inference Thread Terminated\n");
    pthread_exit(NULL);
}

/*****************************************
* Function Name : R_Display_Thread
* Description   : Executes the HDMI Display with Display thread
* Arguments     : threadid = thread identification
* Return value  : -
******************************************/
void *R_Display_Thread(void *threadid)
{
    /*Semaphore Variable*/
    int32_t hdmi_sem_check;
    /*Set Image Info for Imshow*/
    image_info_disp.width  = MIPI_WIDTH;
    image_info_disp.height = MIPI_HEIGHT;
    image_info_disp.format = D_IMAGE_FORMAT_YUYV;
    bool r_image_get = false;
    uint8_t *image = NULL;
    /*Set Display Info*/
    disp_info.output = D_IMAGE_OUTPUT_HDMI;
    disp_info.resolution = D_IMAGE_OUTPUT_AUTO;
    /*Core1 Command Controller Variables*/
    err_t hdmi_isp_err = 0;
    /*Bounding Box Counter*/
    uint8_t bbox_cnt = 0;
    /*First Loop Flag*/
    bool setting_loop = true;
    /*Variable for checking return value*/
    int8_t ret = 0;
    uint32_t i = 0;

    printf("Display Thread Starting\n");

    while (1)
    {
        /*Image Show Ready Flag Monitor*/
        while (1)
        {
            /*Gets the Termination request semaphore value. If different then 1 Termination was requested*/
            /*Checks if sem_getvalue is executed wihtout issue*/
            errno = 0;
            ret = sem_getvalue(&terminate_req_sem, &hdmi_sem_check);
            if (0 != ret)
            {
                fprintf(stderr, "[ERROR] Failed to get Semaphore Value: errno=%d\n", errno);
                goto err;
            }
            /*Checks the semaphore value*/
            if (1 != hdmi_sem_check)
            {
                goto hdmi_end;
            }
            /*Check if Image Show is Ready*/
            if (image_show_ready == image_show_used)
            {
                usleep(WAIT_TIME); //wait 1 tick time
            }
            else
            {
                image_show_used = image_show_ready;
                break;
            }
        }
        /*Get Image Object for HDMI Display*/
        hdmi_isp_err = B_IMAGE_Imread(source_id,&image_info_disp,IMAGE_NUM,&image_obj_disp);
        if (0 != hdmi_isp_err)
        {
            fprintf(stderr, "[ERROR] Failed to run B_IMAGE_Imread(Display Thread): %d\n", hdmi_isp_err);
            goto err;
        }
        usleep(WAIT_TIME); //wait 1 tick time
        /*Graphic Display*/

        /*Gets Memory Address Of Image Stored & Lock Image Object*/
        image = (uint8_t*)B_OBJ_BeginReadingData(image_obj_disp.handle);
        if (NULL == image)
        {
            fprintf(stderr, "[ERROR] Failed to run B_OBJ_BeginReadingData\n");
            goto err;
        }
        else
        {
            /*Save Locked Handle*/
            locked_handle_hdmi = image_obj_disp.handle;
            /*When Successful Read*/
            r_image_get = true;
        }

        usleep(WAIT_TIME); //wait 1 tick time
        hdmi_isp_err = B_IMAGE_ResizeImageData(&image_obj_disp,&image_info_disp,&image_obj_disp_cp);
        if (0 != hdmi_isp_err)
        {
            fprintf(stderr, "[ERROR] Failed to run B_IMAGE_ResizeImageData: %d\n", hdmi_isp_err);
            goto err;
        }

        usleep(WAIT_TIME); //wait 1 tick time
        /*Release Image Object*/
        hdmi_isp_err = B_OBJ_EndReadingData(locked_handle_hdmi);
        if (0 != hdmi_isp_err)
        {
            fprintf(stderr, "[ERROR] Failed to run B_OBJ_EndReadingData: %d\n", hdmi_isp_err);
            goto err;
        }
        else
        {
            r_image_get = false;
        }

        /*Check If First Loop*/
        if (setting_loop)
        {
            /*Set number of layer for Graphic Draw*/
            hdmi_isp_err = B_GRAPH_SetLayerNum(LAYER_NUM, DEBUG_LAYER_NUM);
            if (0 != hdmi_isp_err)
            {
                fprintf(stderr, "[ERROR] Failed to run B_GRAPH_SetLayerNum: %d\n", hdmi_isp_err);
                goto err;
            }
            /*Set Layer Resolution*/
            hdmi_isp_err = B_GRAPH_SetLayerResolution(D_GRAPH_RESOLUTION_960x540);
            if (0 != hdmi_isp_err)
            {
                fprintf(stderr, "[ERROR] Failed to run B_GRAPH_SetLayerResolution: %d\n", hdmi_isp_err);
                goto err;
            }
            /*Set Layer Coordinate Resolution*/
            hdmi_isp_err = B_GRAPH_SetCoordinateReso(D_GRAPH_RESOLUTION_960x540);
            if (0 != hdmi_isp_err)
            {
                fprintf(stderr, "[ERROR] Failed to run B_GRAPH_SetCoordinateReso: %d\n", hdmi_isp_err);
                goto err;
            }
            usleep(WAIT_TIME); //wait 1 tick time
            /*The Layer Setting is only done once*/
            setting_loop = false;
        }
        else
        {
            /*Clear Graphic Drawing Layer*/
            hdmi_isp_err = B_GRAPH_Clear(D_GRAPH_NORMAL_LAYER_1);
            if (0 != hdmi_isp_err)
            {
                fprintf(stderr, "[ERROR] Failed to run B_GRAPH_Clear: %d\n", hdmi_isp_err);
                goto err;
            }
            usleep(WAIT_TIME); //wait 1 tick time
            /*Copy AI Inference Time Info into buffer*/
            sprintf(ai_time_str, "DRP-AI Time:%dms\n", ai_time);
            /*Set AI Inference info text color*/
            hdmi_isp_err = B_GRAPH_SetTextColor(D_GRAPH_NORMAL_LAYER_1,D_GRAPH_COLOR_CYAN);
            if (0 != hdmi_isp_err)
            {
                fprintf(stderr, "[ERROR] Failed to run B_GRAPH_SetTextColor: %d\n", hdmi_isp_err);
                goto err;
            }
            usleep(WAIT_TIME); //wait 1 tick time
            /*Set AI Inference info text size*/
            hdmi_isp_err = B_GRAPH_SetTextSize(D_GRAPH_NORMAL_LAYER_1,FONT_WIDTH, FONT_HEIGHT);
            if (0 != hdmi_isp_err)
            {
                fprintf(stderr, "[ERROR] Failed to run B_GRAPH_SetTextSize: %d\n", hdmi_isp_err);
                goto err;
            }
            usleep(WAIT_TIME); //wait 1 tick time
            /*Set AI Inference info text position*/
            hdmi_isp_err = B_GRAPH_SetTextPosition(D_GRAPH_NORMAL_LAYER_1,AI_TIME_POS_X,AI_TIME_POS_Y);
            if (0 != hdmi_isp_err)
            {
                fprintf(stderr, "[ERROR] Failed to run B_GRAPH_SetTextPosition: %d\n", hdmi_isp_err);
                goto err;
            }
            usleep(WAIT_TIME); //wait 1 tick time
            /*Draw AI Inference info text*/
            hdmi_isp_err = B_GRAPH_DrawText(D_GRAPH_NORMAL_LAYER_1, (int8_t *)(ai_time_str));
            if (0 != hdmi_isp_err)
            {
                fprintf(stderr, "[ERROR] Failed to run B_GRAPH_DrawText: %d\n", hdmi_isp_err);
                goto err;
            }
            usleep(WAIT_TIME); //wait 1 tick time
            /*Clear Bounding Box Count*/
            bbox_cnt = 0;
            /*Draw Bounding Boxes*/
            for (i = 0; i < bcount; i++)
            {
                /* Skip Overlapped Bounding Boxes */
                if (det_res[i].prob == 0)
                {
                    continue;
                }
                /*Copy Detected Class info into buffer*/
                sprintf(inf_res_str,"%s:%.2f\n",label_file_map[det_res[i].c].c_str(),det_res[i].prob);
                /*Get Bounding Box Center Location & Convert it to top left coordinate*/
                x_center = (int16_t) (det_res[i].bbox.x);
                y_center = (int16_t) (det_res[i].bbox.y);
                w_rect = (int16_t) (det_res[i].bbox.w);
                h_rect = (int16_t) (det_res[i].bbox.h);
                /*Modify Coordinate if exceeds the Display Image Size*/
                x_left = x_center - (w_rect / 2);
                y_top = y_center - (h_rect / 2);
                /* Check Bounding Box is within image range*/
                x_left = (x_left < 0                 ) ? 0                    : x_left;
                x_left = (x_left > DISPLAY_WIDTH - 1 ) ? (DISPLAY_WIDTH - 1 ) : x_left;
                y_top  = (y_top  < 0                 ) ? 0                    : y_top;
                y_top  = (y_top  > DISPLAY_HEIGHT - 1) ? (DISPLAY_HEIGHT - 1) : y_top;
                w_rect = (w_rect          < 0                 ) ? 0                            : w_rect;
                w_rect = (w_rect + x_left > DISPLAY_WIDTH - 1 ) ? (DISPLAY_WIDTH - 1 - x_left) : w_rect;
                h_rect = (h_rect          < 0                 ) ? 0                            : h_rect;
                h_rect = (h_rect + y_top  > DISPLAY_HEIGHT - 1) ? (DISPLAY_HEIGHT - 1 - y_top) : h_rect;
                /*Draw Bounding Box*/
                hdmi_isp_err = B_GRAPH_DrawRect(D_GRAPH_NORMAL_LAYER_1,x_left, y_top, w_rect, h_rect);
                if (0 != hdmi_isp_err)
                {
                    fprintf(stderr, "[ERROR] Failed to run B_GRAPH_DrawRect: %d\n", hdmi_isp_err);
                    goto err;
                }
                usleep(WAIT_TIME); //wait 1 tick time
                /*Set Bounding Box Class text position*/
                hdmi_isp_err = B_GRAPH_SetTextPosition(D_GRAPH_NORMAL_LAYER_1,x_left,y_top);
                if (0 != hdmi_isp_err)
                {
                    fprintf(stderr, "[ERROR] Failed to run B_GRAPH_SetTextPosition: %d\n", hdmi_isp_err);
                    goto err;
                }
                usleep(WAIT_TIME); //wait 1 tick time
                /*Draw Bounding Box Class Text*/
                hdmi_isp_err = B_GRAPH_DrawText(D_GRAPH_NORMAL_LAYER_1,(int8_t *)(inf_res_str));
                if (0 != hdmi_isp_err)
                {
                    fprintf(stderr, "[ERROR] Failed to run B_GRAPH_DrawText: %d\n", hdmi_isp_err);
                    goto err;
                }
                usleep(WAIT_TIME); //wait 1 tick time
                /*Due to Graphic API Limitations. Application CANNOT draw more than 5 Bounding Boxes*/
                bbox_cnt++;
                if (BOX_MAX_DISPLAY <= bbox_cnt)
                {
                    break;
                }
            }
            /*Execution of Graphic Display*/
            hdmi_isp_err = B_GRAPH_ExeDisplay(D_GRAPH_NORMAL_LAYER_1);
            if (0 != hdmi_isp_err)
            {
                fprintf(stderr, "[ERROR] Failed to run B_GRAPH_ExeDisplay: %d\n", hdmi_isp_err);
                goto err;
            }
            usleep(WAIT_TIME); //wait 1 tick time
        }
        /*Display on HDMI Monitor*/
        hdmi_isp_err = B_IMAGE_Imshow(&image_obj_disp_cp,&disp_info);
        if (ERR_SYSTEM_ID == hdmi_isp_err)
        {
            fprintf(stdout, "[INFO] B_IMAGE_Imshow Failed: %d\n", hdmi_isp_err);
        }
        else if (0 != hdmi_isp_err)
        {
            fprintf(stderr, "[ERROR] Failed to run B_IMAGE_Imshow: %d\n", hdmi_isp_err);
            goto err;
        }
        usleep(WAIT_TIME); //wait 1 tick time
        hdmi_isp_err = B_OBJ_FreeObject(image_obj_disp_cp.handle);
        if (0 != hdmi_isp_err)
        {
            fprintf(stderr, "[ERROR] Failed to run B_OBJ_FreeObject copied display image object: %d\n", hdmi_isp_err);
            goto err;
        }
    } /*End of Loop*/

/*Error Processing*/
err:
    /*Set Termination Request Semaphore to 0*/
    sem_trywait(&terminate_req_sem);
    goto hdmi_end;
/*Display Thread Termination*/
hdmi_end:
    /*Release Image Object*/
    if (r_image_get)
    {
        B_OBJ_EndReadingData(image_obj.handle);
        r_image_get = false;
    }
    printf("Display Thread Terminated\n");
    pthread_exit(NULL);
}

/*****************************************
* Function Name : R_Kbhit_Thread
* Description   : Executes the Keyboard hit thread (checks if enter key is hit)
* Arguments     : threadid = thread identification
* Return value  : -
******************************************/
void *R_Kbhit_Thread(void *threadid)
{
    /*Semaphore Variable*/
    int32_t kh_sem_check = 0;
    /*Variable to store the getchar() value*/
    int32_t c = 0;
    /*Variable for checking return value*/
    int8_t ret_fcntl = 0;
    int8_t ret = 0;

    printf("Key Hit Thread Starting\n");
    /*Set Standard Input To Non Blocking*/
    errno = 0;
    ret_fcntl = fcntl(0, F_SETFL, O_NONBLOCK);
    if (-1 == ret_fcntl)
    {
        fprintf(stderr, "[ERROR] Failed to run fctnl(): errno=%d\n", errno);
        goto err;
    }

    while (1)
    {
        /*Gets the Termination request semaphore value. If different then 1 Termination was requested*/
        /*Checks if sem_getvalue is executed wihtout issue*/
        errno = 0;
        ret = sem_getvalue(&terminate_req_sem, &kh_sem_check);
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to get Semaphore Value: errno=%d\n", errno);
            goto err;
        }
        /*Checks the semaphore value*/
        if (1 != kh_sem_check)
        {
            goto key_hit_end;
        }
        c = getchar();
        if (EOF != c)
        {
            /* When ENTER is pressed. */
            printf("Enter Detected\n");
            goto err;
        }
        else
        {
            usleep(WAIT_TIME);
        }
    }
/*Error Processing*/
err:
    /*Set Termination Request Semaphore to 0*/
    sem_trywait(&terminate_req_sem);
    goto key_hit_end;
/*Key Hit Thread Termination*/
key_hit_end:
    printf("Key Hit Thread Terminated\n");
    pthread_exit(NULL);
}

/*****************************************
* Function Name : R_Main_Process
* Description   : Runs the main process loop
* Arguments     : -
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
int8_t R_Main_Process()
{
    /*Main Process Variables*/
    int8_t main_ret = 0;
    /*Semaphore Related*/
    int32_t sem_check = 0;
    /*Variable for checking return value*/
    int8_t ret = 0;

    printf("Main Loop Starts\n");
    while (1)
    {
        /*Gets the Termination request semaphore value. If different then 1 Termination was requested*/
        errno = 0;
        ret = sem_getvalue(&terminate_req_sem, &sem_check);
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to get Semaphore Value: errno=%d\n", errno);
            goto err;
        }
        /*Checks the semaphore value*/
        if (1 != sem_check)
        {
            goto end_main_proc;
        }
        usleep(WAIT_TIME); //wait 1 tick time
    } //end main loop

/*Error Processing*/
err:
    /*Set Termination Request Semaphore to 0*/
    sem_trywait(&terminate_req_sem);
    main_ret = -1;
    goto end_main_proc;
/*Main Processing Termination*/
end_main_proc:
    printf("Main Process Terminated\n");
    return main_ret;
}


int32_t main(void)
{
    int8_t main_proc = 0;
    int8_t ret = 0;
    int8_t ret_main = 0;
    /*Multithreading Variables*/
    int32_t create_thread_ai = -1;
    int32_t create_thread_key = -1;
    int32_t create_thread_hdmi = -1;
    int32_t sem_create = -1;
    /*Core1 Command Controller Function Returns*/
    err_t isp_err = 0;

    printf("RZ/V2M DRP-AI Sample Application\n");
    printf("Model : Darknet Tiny YOLOv2 | %s\n", drpai_prefix.c_str());
    printf("Input : MIPI Camera\n");

    /*Initialize Termination Request Semaphore to 1*/
    sem_create = sem_init(&terminate_req_sem,0,1);
    if (0 != sem_create)
    {
        fprintf(stderr, "[ERROR] Failed to initialize Termination Request Semaphore\n");
        goto main_end;
    }
    /*DRP-AI Address Map Reading*/
    ret = read_addrmap_txt(drpai_address_file);
    if (0 != ret)
    {
        fprintf(stderr, "[ERROR] Failed to read addressmap text file: %s\n", drpai_address_file.c_str());
        ret_main = ret;
        goto main_end;
    }
    /*DRP-AI Driver Open*/
    errno = 0;
    drpai_fd = open("/dev/drpai0", O_RDWR);
    if (0 > drpai_fd)
    {
        fprintf(stderr, "[ERROR] Failed to open DRP-AI Driver: errno=%d\n", errno);
        ret_main = -1;
        goto main_end;
    }

    /* Load DRP-AI Data from Filesystem to Memory via DRP-AI Driver */
    ret = load_drpai_data(drpai_fd);
    if (0 != ret)
    {
        fprintf(stderr, "[ERROR] Failed to load DRP-AI Data\n");
        ret_main = ret;
        goto main_end;
    }
    /*Set Frame Info: Video Capture Settings*/
    frame_info.cfg  = D_IMAGE_SIZE_QHD_30P;
    frame_info.n_buffer = MIPI_BUFFER;
    /*Start capturing frames from MIPI Camera*/
    /*Everytime a new frame is ready, the callback will change frame_ready flag*/
    source_id = B_IMAGE_StartVideoCapture(&frame_info,image_frame_callback);
    if (0 == source_id)
    {
        fprintf(stderr, "[ERROR] Failed to run B_IMAGE_StartVideoCapture: %ld\n", source_id);
        ret_main = -1;
        goto main_end;
    }
    /*Create Key Hit Thread*/
    create_thread_key = pthread_create(&kbhit_thread, NULL, R_Kbhit_Thread, NULL);
    if (0 != create_thread_key)
    {
        fprintf(stderr, "[ERROR] Failed to create Key Hit Thread\n");
        ret_main = -1;
        goto main_end;
    }
    /*Create AI Inference Thread*/
    create_thread_ai = pthread_create(&ai_inf_thread, NULL, R_Inf_Thread, NULL);
    if (0 != create_thread_ai)
    {
        sem_trywait(&terminate_req_sem);
        fprintf(stderr, "[ERROR] Failed to create AI Inference Thread\n");
        ret_main = -1;
        goto main_end;
    }
    /*Create Display Thread*/
    create_thread_hdmi = pthread_create(&hdmi_thread, NULL, R_Display_Thread, NULL);
    if (0 != create_thread_hdmi)
    {
        sem_trywait(&terminate_req_sem);
        fprintf(stderr, "[ERROR] Failed to create Display Thread\n");
        ret_main = -1;
        goto main_end;
    }
    /*Run Main Process*/
    main_proc = R_Main_Process();
    if (0 != main_proc)
    {
        fprintf(stderr, "[ERROR] Error during Main Process\n");
        ret_main = -1;
        goto main_end;
    }
    goto main_end;

/*Termination*/
main_end:
    /*Wait till Display Thread Termination*/
    if (0 == create_thread_hdmi)
    {
        ret = wait_join(&hdmi_thread, DISPLAY_THREAD_TIMEOUT);
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to exit Display Thread on time.\n");
            ret_main = -1;
        }
    }
    /*Wait till AI Inference Thread Terminates*/
    if (0 == create_thread_ai)
    {
        ret = wait_join(&ai_inf_thread, AI_THREAD_TIMEOUT);
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to exit AI Inference Thread on time.\n");
            ret_main = -1;
        }
    }
    /*Wait till Key Hit Thread Terminates*/
    if (0 == create_thread_key)
    {
        ret = wait_join(&kbhit_thread, KEY_THREAD_TIMEOUT);
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to exit Key Hit Thread on time.\n");
            ret_main = -1;
        }
    }
    /*Stops Video Capture*/
    if (0 != source_id)
    {
        isp_err = B_IMAGE_StopVideoCapture(source_id);
        if (0 != isp_err)
        {
            fprintf(stderr, "[ERROR] Failed to run B_IMAGE_StopVideoCapture: %d\n", isp_err);
            ret_main = -1;
        }
    }
    /*DRP-AI Driver Close*/
    if (0 <= drpai_fd)
    {
        errno = 0;
        ret = close(drpai_fd);
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to close DRP-AI Driver: errno=%d\n", errno);
            ret_main = -1;
        }
    }
    /*Delete Terminate Request Semaphore*/
    if (0 == sem_create)
    {
        sem_destroy(&terminate_req_sem);
    }

    printf("Application End\n");
    return ret_main;
}
