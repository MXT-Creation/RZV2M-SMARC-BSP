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
* File Name    : sample_app_hrnet_pre-yolov3_cam_hdmi.cpp
* Version      : 7.40
* Description  : RZ/V2M DRP-AI Sample Application for MMPose HRNet with Darknet YOLOv3 MIPI HDMI version
***********************************************************************************************************************/


 /*****************************************
 * Includes
 ******************************************/
/*DRPAI Driver Header*/
#include <linux/drpai.h>
/*Definition of Macros & other variables*/
#include "define.h"
/*Core 1 Command Controller Header*/
#include "comctl/comctl_if.h"
/*YOLOv3 Post-Processing*/
#include "box.h"

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
static float drpai_output_buf[INF_OUT_SIZE];
static uint8_t drpai_output_buf0[INF_OUT_SIZE_YOLO];
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
static std::string prefix;
static addr drpai_address0;
static addr drpai_address;
static std::string dir;
static std::string address_file;

static int drpai_fd = -1;
static int drpai_fd0 = -1;
static uint32_t ai_time;
static uint32_t yolo_time;
static uint32_t hrnet_time[NUM_MAX_PERSON];
static char ai_time_str[DISPLAY_TEXT_SIZE];
static float hrnet_preds[NUM_OUTPUT_C][3];
static uint16_t id_x[NUM_OUTPUT_C][NUM_MAX_PERSON];
static uint16_t id_y[NUM_OUTPUT_C][NUM_MAX_PERSON];
static uint16_t id_x_local[NUM_OUTPUT_C][NUM_MAX_PERSON]; /*To be used only in Inference Threads*/
static uint16_t id_y_local[NUM_OUTPUT_C][NUM_MAX_PERSON]; /*To be used only in Inference Threads*/
static int16_t cropx[NUM_MAX_PERSON];
static int16_t cropy[NUM_MAX_PERSON];
static int16_t croph[NUM_MAX_PERSON];
static int16_t cropw[NUM_MAX_PERSON];
static int16_t output_left[NUM_MAX_PERSON];
static int16_t output_top[NUM_MAX_PERSON];
static int16_t output_h[NUM_MAX_PERSON];
static int16_t output_w[NUM_MAX_PERSON];
static float lowest_kpt_score[NUM_MAX_PERSON];
static float lowest_kpt_score_local[NUM_MAX_PERSON]; /*To be used only in Inference Threads*/
/*YOLOv3*/
static uint32_t bcount = 0;
static uint32_t ppl_count_local = 0; /*To be used only in Inference Threads*/
static uint32_t ppl_count = 0;
static std::vector<detection> det_res;
static std::vector<detection> det_ppl;
static std::vector<std::string> label_file_map;

/*****************************************
* Function Name : wait_join
* Description   : waits for a fixed amount of time for the thread to exit
* Arguments     : p_join_thread = thread that the function waits for to Exit
*                 join_time = the timeout time for the thread for exiting
* Return value  : 0 if successful
*                 not 0 otherwise
******************************************/
static int wait_join(pthread_t *p_join_thread, uint32_t join_time)
{
    int ret_err;
    struct timespec join_timeout;
    ret_err = clock_gettime(CLOCK_REALTIME, &join_timeout);
    if( ret_err == 0 )
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
* Description   : helper function for YOLO Post Processing
* Arguments     : x = input argument for the calculation
* Return value  : sigmoid result of input x
******************************************/
double sigmoid(double x)
{
    return 1.0/(1.0+exp(-x));
}

/*****************************************
* Function Name : softmax
* Description   : helper function for YOLO Post Processing
* Arguments     : val[] = array to be computed softmax
* Return value  : -
******************************************/
void softmax(float val[NUM_CLASS])
{
    float max = -INT_MAX;
    float sum = 0;
    int i;
    for ( i = 0 ; i<NUM_CLASS ; i++ )
    {
        max = std::max(max, val[i]);
    }

    for ( i = 0 ; i<NUM_CLASS ; i++ )
    {
        val[i]= (float) exp(val[i] - max);
        sum+= val[i];
    }

    for ( i = 0 ; i<NUM_CLASS ; i++ )
    {
        val[i]= val[i]/sum;
    }
}

/*****************************************
* Function Name : index
* Description   : Get the index of the bounding box attributes based on the input offset
* Arguments     : offs = offset to access the bounding box attributes
*                 channel = channel to access each bounding box attribute.
* Return value  : index to access the bounding box attribute.
******************************************/
int index(uint8_t n, int offs, int channel)
{
    uint8_t num_grid = 0;
    switch (n)
    {
        case 0:
            num_grid = NUM_GRID_1;
            break;
        case 1:
            num_grid = NUM_GRID_2;
            break;
        case 2:
            num_grid = NUM_GRID_3;
            break;
        default:
            break;
    }
    return offs + channel * num_grid * num_grid;
}

/*****************************************
* Function Name : offset_yolo
* Description   : Get the offset number to access the bounding box attributes
*                 To get the actual value of bounding box attributes, use index() after this function.
* Arguments     : b = Number to indicate which bounding box in the region [0~4]
*                 y = Number to indicate which region [0~13]
*                 x = Number to indicate which region [0~13]
* Return value  : offset to access the bounding box attributes.
*******************************************/
int offset_yolo(uint8_t n, int b, int y, int x)
{
    uint8_t num_grid = 0;
    uint32_t prev_layer_num = 0;
    switch (n)
    {
        case 0:
            num_grid = NUM_GRID_1;
            break;
        case 1:
            num_grid = NUM_GRID_2;
            prev_layer_num += NUM_BB *(NUM_CLASS + 5)* NUM_GRID_1 * NUM_GRID_1;
            break;
        case 2:
            num_grid = NUM_GRID_3;
            prev_layer_num += NUM_BB *(NUM_CLASS + 5)* NUM_GRID_1 * NUM_GRID_1;
            prev_layer_num += NUM_BB *(NUM_CLASS + 5)* NUM_GRID_2 * NUM_GRID_2;
            break;
        default:
            break;
    }
    return prev_layer_num + b *(NUM_CLASS + 5)* num_grid * num_grid + y * num_grid + x;
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
    for(uint32_t i = 0; i<box_count; i++)
    {
        if(det[i].prob == 0)
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
* Description   : Process CPU post-processing for YOLOv3
* Arguments     : address = drpai output address
*                 det = detected boxes details
*                 box_count = total number of boxes
* Return value  : -
******************************************/
void R_Post_Proc(uint8_t* address, std::vector<detection>& det, uint32_t* box_count)
{
    uint32_t count = 0;
    /*Memory Access*/
    float* floatarr;
    /* Following variables are required for correct_yolo_boxes in Darknet implementation*/
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

    int n, b, y, x, offs, i;
    float tx, ty, tw, th, tc;
    float center_x, center_y, box_w, box_h, objectness;
    uint8_t num_grid, anchor_offset;
    floatarr = (float*)address;
    /* Clear the detected result list */
    det.clear();

    for (n = 0; n<NUM_OUT_LAYER; n++)
    {
        num_grid = 0;
        anchor_offset = 2 * NUM_BB * (NUM_BB - (n + 1));
        switch (n)
        {
            case 0:
                num_grid = NUM_GRID_1;
                break;
            case 1:
                num_grid = NUM_GRID_2;
                break;
            case 2:
                num_grid = NUM_GRID_3;
                break;
            default:
                break;
        }

        for(b = 0;b<NUM_BB;b++)
        {
            for(y = 0;y<num_grid;y++)
            {
                for(x = 0;x<num_grid;x++)
                {
                    offs = offset_yolo(n, b, y, x);
                    tx = floatarr[offs];
                    ty = floatarr[index(n, offs, 1)];
                    tw = floatarr[index(n, offs, 2)];
                    th = floatarr[index(n, offs, 3)];
                    tc = floatarr[index(n, offs, 4)];

                    /* Compute the bounding box */
                    /*get_yolo_box in paper implementation*/
                    center_x = ((float) x + sigmoid(tx)) / (float) num_grid;
                    center_y = ((float) y + sigmoid(ty)) / (float) num_grid;
                    box_w = (float) exp(tw) * anchors[anchor_offset+2*b+0] / (float) MODEL_IN_W;
                    box_h = (float) exp(th) * anchors[anchor_offset+2*b+1] / (float) MODEL_IN_W;

                    /* Adjustment for VGA size */
                    /* correct_yolo_boxes */
                    center_x = (center_x - (MODEL_IN_W - new_w) / 2. / MODEL_IN_W) / ((float) new_w / MODEL_IN_W);
                    center_y = (center_y - (MODEL_IN_H - new_h) / 2. / MODEL_IN_H) / ((float) new_h / MODEL_IN_H);
                    box_w *= (float) (MODEL_IN_W / new_w);
                    box_h *= (float) (MODEL_IN_H / new_h);

                    center_x = round(center_x * IMREAD_IMAGE_WIDTH);
                    center_y = round(center_y * IMREAD_IMAGE_HEIGHT);
                    box_w = round(box_w * IMREAD_IMAGE_WIDTH);
                    box_h = round(box_h * IMREAD_IMAGE_HEIGHT);

                    objectness = sigmoid(tc);

                    Box bb = {center_x, center_y, box_w, box_h};
                    /* Get the class prediction */
                    float classes[NUM_CLASS];
                    for (int c = 0;c < NUM_CLASS;c++)
                    {
                        classes[c] = sigmoid(floatarr[index(n, offs, 5+c)]);
                    }
                    float max_pred = 0;
                    int pred_class = -1;
                    for (int c = 0;c < NUM_CLASS;c++)
                    {
                        if (classes[c] > max_pred)
                        {
                            pred_class = c;
                            max_pred = classes[c];
                        }
                    }

                    /* Store the result into the list if the probability is more than the threshold */
                    float probability = max_pred * objectness;
                    if ((probability > TH_PROB) && (pred_class == PERSON_LABEL_NUM))
                    {
                        detection d = {bb, pred_class, probability};
                        det.push_back(d);
                    }
                }
            }
        }
    }
    /* Non-Maximum Supression filter */
    filter_boxes_nms(det, det.size(), TH_NMS);
    count = det.size();
    *box_count = count;
}


/*****************************************
* Function Name : people_counter
* Description   : Function to count the real number of people detected and does not exceeds the maximum number
* Arguments     : det = detected boxes details
*                 ppl = detected people details
*                 box_count = total number of boxes
*                 ppl_count = actual number of people
* Return value  : -
******************************************/
void people_counter(std::vector<detection>& det, std::vector<detection>& ppl, uint32_t box_count, uint32_t* ppl_count)
{
    uint32_t count = 0;
    ppl.clear();
    for(uint32_t i = 0; i<box_count; i++)
    {
        if(det[i].prob == 0)
        {
            continue;
        }
        else
        {
            ppl.push_back(det[i]);
            count++;
            if(count > NUM_MAX_PERSON-1)
            {
                break;
            }
        }
    }
    *ppl_count = count;
}

/*****************************************
* Function Name : offset_hrnet
* Description   : Get the offset number to access the HRNet attributes
* Arguments     : b = Number to indicate which region [0~17]
*                 y = Number to indicate which region [0~64]
*                 x = Number to indicate which region [0~48]
* Return value  : offset to access the HRNet attributes.
*******************************************/
int32_t offset_hrnet(int32_t b, int32_t y, int32_t x)
{
    return b * NUM_OUTPUT_W * NUM_OUTPUT_H + y * NUM_OUTPUT_W + x;
}

/*****************************************
* Function Name : sign
* Description   : Get the sign of the input value
* Arguments     : x = input value
* Return value  : returns the sign, 1 if positive -1 if not
*******************************************/
int8_t sign(int32_t x)
{
    return x > 0 ? 1 : -1;
}

/*****************************************
* Function Name : R_Post_Proc_HRNet
* Description   : CPU post-processing for HRNet
*                 Microsoft COCO: Common Objects in Context' ECCV'2014
*                 More details can be found in the `paper
*                 <https://arxiv.org/abs/1405.0312>
*                 COCO Keypoint Indexes:
*                 0: 'nose',
*                 1: 'left_eye',
*                 2: 'right_eye',
*                 3: 'left_ear',
*                 4: 'right_ear',
*                 5: 'left_shoulder',
*                 6: 'right_shoulder',
*                 7: 'left_elbow',
*                 8: 'right_elbow',
*                 9: 'left_wrist',
*                 10: 'right_wrist',
*                 11: 'left_hip',
*                 12: 'right_hip',
*                 13: 'left_knee',
*                 14: 'right_knee',
*                 15: 'left_ankle',
*                 16: 'right_ankle'
* Arguments     : floatarr = drpai output address
*                 n_pers = number of the person detected
* Return value  : -
******************************************/
void R_Post_Proc_HRNet(float* floatarr, uint8_t n_pers)
{
    float score;
    int32_t b, y, x, offs;
    float center[] = {(float)(cropw[n_pers] / 2 -1), (float)(croph[n_pers] / 2 - 1)};
    int8_t ind_x = -1;
    int8_t ind_y = -1;
    float max_val = -1;
    float scale_x, scale_y, coords_x, coords_y;
    for(b = 0;b<NUM_OUTPUT_C;b++)
    {
        float scale[] = {(float)(cropw[n_pers] / 200.0), (float)(croph[n_pers] / 200.0)};
        ind_x = -1;
        ind_y = -1;
        max_val = -1;
        for(y = 0;y<NUM_OUTPUT_H;y++)
        {
            for(x = 0;x<NUM_OUTPUT_W;x++)
            {
                offs = offset_hrnet(b, y, x);
                if (floatarr[offs] > max_val )
                {
                    /*Update the maximum value and indices*/
                    max_val = floatarr[offs];
                    ind_x = x;
                    ind_y = y;
                }
            }
        }
        if (max_val < 0)
        {
            ind_x = -1;
            ind_y = -1;
            goto not_detect;
        }
        hrnet_preds[b][0] = float(ind_x);
        hrnet_preds[b][1] = float(ind_y);
        hrnet_preds[b][2] = max_val;
        offs = offset_hrnet(b, ind_y, ind_x);
        if (ind_y > 1 && ind_y < NUM_OUTPUT_H -1)
        {
            if (ind_x > 1 && ind_x < NUM_OUTPUT_W -1)
            {
                float diff_x = floatarr[offs + 1] - floatarr[offs - 1];
                float diff_y = floatarr[offs + NUM_OUTPUT_W] - floatarr[offs - NUM_OUTPUT_W];
                hrnet_preds[b][0] += sign(diff_x) * 0.25;
                hrnet_preds[b][1] += sign(diff_y) * 0.25;
            }
        }

        /*transform_preds*/
        scale[0] *= 200;
        scale[1] *= 200;
        //udp (Unbiased Data Processing) = False
        scale_x = scale[0] / (NUM_OUTPUT_W);
        scale_y = scale[1] / (NUM_OUTPUT_H);
        coords_x = hrnet_preds[b][0];
        coords_y = hrnet_preds[b][1];
        hrnet_preds[b][0] = coords_x * scale_x + center[0] - scale[0] * 0.5;
        hrnet_preds[b][1] = coords_y * scale_y + center[1] - scale[1] * 0.5;
    }
    /* Clear the score in preparation for the update. */
    lowest_kpt_score_local[n_pers] = 0;
    score = 1;
    for (int8_t i = 0;i < NUM_OUTPUT_C; i++)
    {
        /* Adopt the lowest score. */
        if (hrnet_preds[i][2] < score)
        {
            score = hrnet_preds[i][2];
        }
    }
    /* Update the score for display thread. */
    lowest_kpt_score_local[n_pers] = score;
    goto end;

not_detect:
    lowest_kpt_score_local[n_pers] = 0;
    goto end;

end:
    return;
}

/*****************************************
* Function Name : R_HRNet_Coord_Convert
* Description   : Convert the post processing result into drawable coordinates
* Arguments     : n_pers = number of the detected person
* Return value  : -
******************************************/
void R_HRNet_Coord_Convert(uint8_t n_pers)
{
    /* Render skeleton on image and print their details */
    int32_t posx;
    int32_t posy;

    printf("\nResult-------------------------------------\n");
    for (int8_t i = 0;i < NUM_OUTPUT_C; i++)
    {
        /* Print out keypoint details */
        printf("  ID %d: (%.2f, %.2f): %.2f%%",i, hrnet_preds[i][0], hrnet_preds[i][1], hrnet_preds[i][2] * 100);
        /* Conversion from input image coordinates to display image coordinates. */
        /* +0.5 is for rounding.*/
        posx = (int32_t)(hrnet_preds[i][0] / cropw[n_pers] * output_w[n_pers] + 0.5) + output_left[n_pers] + OUTPUT_ADJ_X;
        posy = (int32_t)(hrnet_preds[i][1] / croph[n_pers] * output_h[n_pers] + 0.5) + output_top[n_pers] + OUTPUT_ADJ_Y;
        /* Make sure the coordinates are not off the screen. */
        posx = (posx < 0) ? 0 : posx;
        posx = (posx > DISPLAY_WIDTH - KEY_POINT_SIZE -1 ) ? DISPLAY_WIDTH -KEY_POINT_SIZE -1 : posx;
        posy = (posy < 0) ? 0 : posy;
        posy = (posy > DISPLAY_HEIGHT -KEY_POINT_SIZE -1) ? DISPLAY_HEIGHT -KEY_POINT_SIZE -1 : posy;

        id_x_local[i][n_pers] = posx;
        id_y_local[i][n_pers] = posy;
        printf(" -> (%d, %d)\n", id_x_local[i][n_pers], id_y_local[i][n_pers]);
    }
    return;
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
*                 address_drp = drp address map to fill
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
uint8_t read_addrmap_txt(std::string addr_file, addr* address_drp)
{
    uint8_t ret_read = 0;
    std::ifstream ifs(addr_file);
    std::string str;
    std::string element, a, s;
    uint32_t l_addr;
    uint32_t l_size;

    if (ifs.fail())
    {
        ret_read = ERR_OPEN;
        goto err_ifs;
    }
    while(getline(ifs, str))
    {
        std::istringstream iss(str);
        iss >> element >> a >> s;
        l_addr = strtol(a.c_str(), NULL, 16);
        l_size = strtol(s.c_str(), NULL, 16);

        if (element == "drp_config")
        {
            address_drp->drp_config_addr = l_addr;
            address_drp->drp_config_size = l_size;
        }
        else if (element == "desc_aimac")
        {
            address_drp->desc_aimac_addr = l_addr;
            address_drp->desc_aimac_size = l_size;
        }
        else if (element == "desc_drp")
        {
            address_drp->desc_drp_addr = l_addr;
            address_drp->desc_drp_size = l_size;
        }
        else if (element == "drp_param")
        {
            address_drp->drp_param_addr = l_addr;
            address_drp->drp_param_size = l_size;
        }
        else if (element == "weight")
        {
            address_drp->weight_addr = l_addr;
            address_drp->weight_size = l_size;
        }
        else if (element == "data_in")
        {
            address_drp->data_in_addr = l_addr;
            address_drp->data_in_size = l_size;
        }
        else if (element == "data")
        {
            address_drp->data_addr = l_addr;
            address_drp->data_size = l_size;
        }
        else if (element == "data_out")
        {
            address_drp->data_out_addr = l_addr;
            address_drp->data_out_size = l_size;
        }
        else if (element == "work")
        {
            address_drp->work_addr = l_addr;
            address_drp->work_size = l_size;
        }
    }
    goto end;

err_ifs:
    printf("\x1b[31;1m[ERROR] Address Map List %s\n\x1b[0m",addr_file.c_str());
    goto end;
end:
    return ret_read;
}

/*****************************************
* Function Name : load_data_to_mem
* Description   : Loads a file to memory via DRP-AI Driver
* Arguments     : data = filename to be written to memory
*               : drpai_fd = file descriptor of DRP-AI Driver
*               : from = memory start address where the data is written
*               : size = data size to be written
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
uint8_t load_data_to_mem(std::string data, int drpai_fd, uint32_t from, uint32_t size)
{
    uint8_t ret_load_data = SUCCESS;
    int obj_fd;
    drpai_data_t drpai_data;

    printf("[START] Loading %s: size %X at address %X\n", data.c_str(), size, from);
    obj_fd = open(data.c_str(), O_RDONLY);
    if (obj_fd < 0)
    {
        ret_load_data = ERR_OPEN;
        goto end;
    }
    drpai_data.address = from;
    drpai_data.size = size;
    if (0 != ioctl(drpai_fd, DRPAI_ASSIGN, &drpai_data))
    {
        ret_load_data = ERR_DRPAI_ASSIGN;
        goto err_drpai_assign;
    }
    for (uint32_t i = 0 ; i<(drpai_data.size/BUF_SIZE) ; i++)
    {
        if( SUCCESS > read(obj_fd, drpai_buf, BUF_SIZE))
        {
            ret_load_data = ERR_READ;
            goto end;
        }
        if ( SUCCESS > write(drpai_fd, drpai_buf,  BUF_SIZE))
        {
            ret_load_data = ERR_DRPAI_WRITE;
            goto err_drpai_write;
        }
    }
    if ( 0 != (drpai_data.size%BUF_SIZE))
    {
        if( SUCCESS > read(obj_fd, drpai_buf, (drpai_data.size % BUF_SIZE)))
        {
            ret_load_data = ERR_READ;
            goto end;
        }
        if( SUCCESS > write(drpai_fd, drpai_buf, (drpai_data.size % BUF_SIZE)))
        {
            ret_load_data = ERR_DRPAI_WRITE;
            goto err_drpai_write;
        }
    }
    goto end;

err_drpai_assign:
  printf("[ERROR] DRPAI Assign Failed:%d\n", errno);
  goto end;
err_drpai_write:
  printf("[ERROR] DRPAI Write Failed:%d\n", errno);
  goto end;
end:
  if(obj_fd >= 0)
  {
        close(obj_fd);
  }
  if(ret_load_data == SUCCESS)
  {
      printf("[END] Loading  %s\n", data.c_str());
  }
  return ret_load_data;
}

/*****************************************
* Function Name :  load_drpai_data
* Description   : Loads DRP-AI Object files to memory via DRP-AI Driver.
* Arguments     : drpai_fd = file descriptor of DRP-AI Driver
*                 address_drp = drp address map
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
uint8_t load_drpai_data(int drpai_fd, addr address_drp)
{
    uint8_t ret = SUCCESS;
    uint64_t addr, size;
    double diff = 0;
    std::string drpai_file_path[5] =
    {
        prefix+"/drp_desc.bin",
        prefix+"/"+prefix+"_drpcfg.mem",
        prefix+"/drp_param.bin",
        prefix+"/aimac_desc.bin",
        prefix+"/"+prefix+"_weight.dat",
    };

    struct timespec start_time, stop_time;
    if(timespec_get(&start_time, TIME_UTC)==0)//Start load time
    {
       ret = ERR_GET_TIME;
       goto end;
    }
    printf("[START] Loading DRP-AI Data...\n");
    for (uint8_t i = 0;i<5;i++)
    {
        switch (i)
        {
            case (INDEX_W):
                addr = address_drp.weight_addr;
                size = address_drp.weight_size;
                break;
            case (INDEX_C):
                addr = address_drp.drp_config_addr;
                size = address_drp.drp_config_size;
                break;
            case (INDEX_P):
                addr = address_drp.drp_param_addr;
                size = address_drp.drp_param_size;
                break;
            case (INDEX_A):
                addr = address_drp.desc_aimac_addr;
                size = address_drp.desc_aimac_size;
                break;
            case (INDEX_D):
                addr = address_drp.desc_drp_addr;
                size = address_drp.desc_drp_size;
                break;
            default:
                break;
        }
        ret = load_data_to_mem(drpai_file_path[i], drpai_fd, addr, size);
        if (ret > SUCCESS )
        {
            printf("\x1b[31;1m[ERROR] load_data_to_mem failed : %s\n\x1b[0m",drpai_file_path[i].c_str());
            goto end;
        }
    }
    if(timespec_get(&stop_time, TIME_UTC)==0)//Stop load time
    {
       ret = ERR_GET_TIME;
       goto end;
    }
    diff = timedifference_msec(start_time,stop_time);
    printf("[END] Loading DRP-AI Data : Total loading time %f s\n", diff*0.001);
    goto end;

end:
    return ret;
}
std::vector<std::string> load_label_file(std::string label_file_name)
{
    int n = 0;
    std::vector<std::string> list;
    std::ifstream infile(label_file_name);

    if (!infile.is_open())
    {
        return list;
    }

    std::string line;
    while (getline(infile,line))
    {
        list.push_back(line);
        if (infile.fail())
        {
            std::vector<std::string> empty;
            return empty;
        }
    }

    return list;
}

/*****************************************
* Function Name :  load_drpai_param_file
* Description   : Loads DRP-AI Parameter File to memory via DRP-AI Driver.
* Arguments     : drpai_fd = file descriptor of DRP-AI Driver
*                 proc = drpai data
*                 param_file = drpai parameter file to load
*                 file_size = drpai parameter file size
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
uint8_t load_drpai_param_file(int drpai_fd, drpai_data_t proc, std::string param_file, uint32_t file_size)
{
    uint8_t ret_load_para = SUCCESS;
    int obj_fd = -1;
    drpai_assign_param_t crop_assign;
    crop_assign.info_size = file_size;
    crop_assign.obj.address = proc.address;
    crop_assign.obj.size = proc.size;
    if (ioctl(drpai_fd, DRPAI_ASSIGN_PARAM, &crop_assign)!=0)
    {
        ret_load_para = ERR_DRPAI_ASSIGN_PARAM;
        goto err_drpai_assign_param;
    }
    obj_fd = open(param_file.c_str(), O_RDONLY);
    if (obj_fd < 0)
    {
        ret_load_para = ERR_OPEN;
        goto end;
    }
    for(uint32_t i = 0 ; i<(file_size/BUF_SIZE) ; i++)
    {
        if( SUCCESS > read(obj_fd, drpai_buf, BUF_SIZE))
        {
            ret_load_para = ERR_READ;
            goto end;
        }
        if ( SUCCESS > write(drpai_fd, drpai_buf, BUF_SIZE))
        {
            ret_load_para = ERR_DRPAI_WRITE;
            goto err_drpai_write;
        }
    }
    if ( 0 != (file_size%BUF_SIZE))
    {
        if( SUCCESS > read(obj_fd, drpai_buf, (file_size % BUF_SIZE)))
        {
            ret_load_para = ERR_READ;
            goto end;
        }
        if( SUCCESS > write(drpai_fd, drpai_buf, (file_size % BUF_SIZE)))
        {
            ret_load_para = ERR_DRPAI_WRITE;
            goto err_drpai_write;
        }
    }
    goto end;

err_drpai_assign_param:
    printf("[ERROR] DRPAI Assign Parameter Failed:%d\n", errno);
    goto end;
err_drpai_write:
    printf("[ERROR] DRPAI Write Failed:%d\n", errno);
    goto end;
end:
    if(obj_fd >= 0)
    {
        close(obj_fd);
    }
    return ret_load_para;
}

/*****************************************
* Function Name : get_input
* Description   : Get DRP-AI Input from memory via DRP-AI Driver
* Arguments     : mem_fd = file descriptor of memory device file
* Return value  : 0 if succeeded
*                 not 0 otherwise
******************************************/
uint8_t get_input(int mem_fd)
{
    uint8_t ret = SUCCESS;
    /*Core1 Command Controller Variables*/
    bool r_image_get = false;
    err_t inf_isp_err = 0;
    uint8_t *image = NULL;
    /*For Image Copy*/
    uint8_t* drpai_img_map = (uint8_t*)MAP_FAILED;
    uint32_t drpai_img_size = IMREAD_IMAGE_SIZE;
    uint32_t drpai_img_addr;
    uint32_t drpai_img_start_addr;
    uint32_t drpai_img_offset;

    /*Get Image Object For AI Inference*/
    inf_isp_err = B_IMAGE_Imread(source_id,&image_info,IMAGE_NUM,&image_obj);
    if(inf_isp_err != 0)
    {
        goto err_imread;
    }
    /*Gets Memory Address Of Image Stored & Lock Image Object*/
    image = (uint8_t*)B_OBJ_BeginReadingData(image_obj.handle);
    if(image == NULL)
    {
        goto err_getimg;
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
    if (drpai_img_map == MAP_FAILED)
    {
        goto err_mmap_img;
    }
    /*write copy image to drpai*/
    if (SUCCESS != ioctl(drpai_fd, DRPAI_ASSIGN, &drpai_data_in[idata_idx]))
    {
        goto err_drpai_assign_input;
    }
    if(SUCCESS > write(drpai_fd, &drpai_img_map[drpai_img_offset], drpai_data_in[idata_idx].size))
    {
        goto err_drpai_write_input;
    }

    /*unmap the Image Virtual Address*/
    munmap(drpai_img_map,(size_t)(drpai_img_size+drpai_img_offset));
    drpai_img_map = (uint8_t*)MAP_FAILED;
    /*Release Image Object*/
    inf_isp_err = B_OBJ_EndReadingData(locked_handle);
    if(inf_isp_err != 0)
    {
        goto err_release_img;
    }
    else
    {
        r_image_get = false;
    }
    goto end;

err_imread:
    printf("\x1b[31;1m[ERROR] B_IMAGE_Imread Failed: %d\n\x1b[0m", inf_isp_err);
    goto err;
err_gettime_info:
    printf("\x1b[31;1m[ERROR] B_OBJ_GetTimeInfo Failed: %d\n\x1b[0m", inf_isp_err);
    goto err;
err_getimg:
    printf("\x1b[31;1m[ERROR] B_OBJ_BeginReadingData Failed\n\x1b[0m");
    goto err;
err_mmap_img:
    printf("\x1b[31;1m[ERROR] Failed to mmap /dev/mem for Captured Image Frame\n\x1b[0m");
    goto err;
err_drpai_assign_input:
    printf("\x1b[31;1m[ERROR] Failed to assign DRPAI Input data: %d\n\x1b[0m",errno);
    goto err;
err_drpai_write_input:
    printf("\x1b[31;1m[ERROR] Failed to write DRPAI Input data: %d\n\x1b[0m", errno);
    goto err;
err_release_img:
    printf("\x1b[31;1m[ERROR] B_OBJ_EndReadingData Failed: %d\n\x1b[0m", inf_isp_err);
    goto err;
err:
    ret = 1;
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
    int inf_sem_check;
    /*Core1 Command Controller Variables*/
    err_t inf_isp_err = 0;
    bool r_image_get = false;
    uint8_t *image = NULL;
    /*Variable For getting Inference Output Data*/
    drpai_data_t drpai_data;
    drpai_data_t drpai_data0;
    /*Inference Variables*/
    fd_set rfds;
    fd_set rfds0;
    struct timespec tv;
    sigset_t sigset;
    int ret_drpai;
    int inf_status = 0;
    drpai_data_t proc0[DRPAI_INDEX_NUM];
    drpai_data_t proc[DRPAI_INDEX_NUM];
    int32_t inf_cnt = -1;
    drpai_status_t drpai_status;
    int32_t idata_swap_flag = 0;
    /*Variable For Performance Measurement*/
    static struct timespec start_time;
    static struct timespec inf_end_time;
    /*Callback Timeout Counter*/
    uint32_t cb_time_cnt = 0;
    /*HRNet Modify Parameters*/
    drpai_crop_t crop_param;
    static std::string drpai_param_file;
    uint32_t drp_param_info_size;
    static std::string labels = label_list;
    /*Setting SIGUSR1 Signal*/
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);

    printf("Inference Thread Starting\n");
    /*HRNet*/
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
    /*YOLOv3*/
    proc0[DRPAI_INDEX_INPUT].address = drpai_address0.data_in_addr;
    proc0[DRPAI_INDEX_INPUT].size = drpai_address0.data_in_size;
    proc0[DRPAI_INDEX_DRP_CFG].address = drpai_address0.drp_config_addr;
    proc0[DRPAI_INDEX_DRP_CFG].size = drpai_address0.drp_config_size;
    proc0[DRPAI_INDEX_DRP_PARAM].address = drpai_address0.drp_param_addr;
    proc0[DRPAI_INDEX_DRP_PARAM].size = drpai_address0.drp_param_size;
    proc0[DRPAI_INDEX_AIMAC_DESC].address = drpai_address0.desc_aimac_addr;
    proc0[DRPAI_INDEX_AIMAC_DESC].size = drpai_address0.desc_aimac_size;
    proc0[DRPAI_INDEX_DRP_DESC].address = drpai_address0.desc_drp_addr;
    proc0[DRPAI_INDEX_DRP_DESC].size = drpai_address0.desc_drp_size;
    proc0[DRPAI_INDEX_WEIGHT].address = drpai_address0.weight_addr;
    proc0[DRPAI_INDEX_WEIGHT].size = drpai_address0.weight_size;
    proc0[DRPAI_INDEX_OUTPUT].address = drpai_address0.data_out_addr;
    proc0[DRPAI_INDEX_OUTPUT].size = drpai_address0.data_out_size;
    /*For Image Copy*/
    int mem_fd = -1;
    /*DRP-AI Output Memory Preparation*/
    /*HRNet*/
    drpai_data.address = drpai_address.data_out_addr;
    drpai_data.size = drpai_address.data_out_size;
    /*YOLOv3*/
    drpai_data0.address = drpai_address0.data_out_addr;
    drpai_data0.size = drpai_address0.data_out_size;
    /*DRP-AI Input Memory Preparation*/
    drpai_data_in[0].address = drpai_address.data_in_addr;
    drpai_data_in[0].size = drpai_address.data_in_size;
    drpai_data_in[1].address = SECOND_INPUT_ADDR;
    drpai_data_in[1].size = drpai_address.data_in_size;
    /*Set Image info for AI Inference*/
    image_info.width  = IMREAD_IMAGE_WIDTH;
    image_info.height = IMREAD_IMAGE_HEIGHT;
    image_info.format = D_IMAGE_FORMAT_YUYV;
    /*DRP Param Info Preparation*/
    prefix = AI_DESC_NAME;
    dir = prefix + "/";
    drpai_param_file = dir + "drp_param_info.txt";
    std::ifstream param_file(drpai_param_file, std::ifstream::ate);
    drp_param_info_size = static_cast<uint32_t>(param_file.tellg());
    /*Load DRPAI Parameter for Cropping later*/
    if(load_drpai_param_file(drpai_fd, proc[DRPAI_INDEX_DRP_PARAM], drpai_param_file, drp_param_info_size) != SUCCESS)
    {
        goto err_load_drpai_param;
    }
    /*Load Label*/
    label_file_map = load_label_file(labels);
    if (label_file_map.empty())
    {
        fprintf(stderr,"[ERROR] load_label_file failed : %s\n",labels.c_str());
        goto err;
    }
    /*Open Memory Access*/
    mem_fd = open("/dev/mem", O_RDWR);
    if (mem_fd <= 0)
    {
        goto err_open_mem_img;
    }
    printf("Inference Loop Starting\n");
    /*Inference Loop Start*/
    while(1)
    {
        if (0 == idata_swap_flag)
        {
            /*Image Ready Flag Monitor*/
            while(1)
            {
                /*Gets The Termination Request Semaphore Value, If Different Then 1 Termination Was Requested*/
                /*Checks If sem_getvalue Is Executed Without Issue*/
                if(sem_getvalue(&terminate_req_sem, &inf_sem_check) != 0)
                {
                    goto err_getsem;
                }
                /*Checks The Semaphore Value*/
                if(inf_sem_check != 1)
                {
                    goto ai_inf_end;
                }
                /*Check If Image Frame Is Ready*/
                if(image_frame_ready == image_frame_used)
                {
                    usleep(WAIT_TIME); //wait 1 tick time
                    /*Callback Timeout Counter Up*/
                    cb_time_cnt++;
                    if(cb_time_cnt >= CALLBACK_TIMEOUT)
                    {
                        goto err_cb_timeout;
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
            /*Reset Video Capture Callback Timeout Counter*/
            cb_time_cnt = 0;
            /*Copy input image to expanded area*/
            if(SUCCESS != get_input(mem_fd))
            {
                goto err_get_input;
            }
        }
        /*Writes Data Input Physical Address & Size Directly to DRP-AI*/
        proc[DRPAI_INDEX_INPUT] = drpai_data_in[idata_idx];
        proc0[DRPAI_INDEX_INPUT] = proc[DRPAI_INDEX_INPUT];
        /*Run Inference*/
        /*Get Inference Start Time*/
        if(timespec_get(&start_time, TIME_UTC) == 0)
        {
            goto err_gettime_start;
        }
        /*Start DRP-AI Driver*/
        if (SUCCESS != ioctl(drpai_fd0, DRPAI_START, &proc0[0]))
        {
            goto err_start_inf;
        }
        inf_cnt++;
        printf("[START] Start DRP-AI inference...\n");
        printf("Inference ----------- No. %d\n",(inf_cnt+1));
        /*Reset swap flag*/
        idata_swap_flag = 0;
        while(1)
        {
            /*Gets The Termination Request Semaphore Value, If Different Then 1 Termination Was Requested*/
            /*Checks If sem_getvalue Is Executed Without Issue*/
            if(sem_getvalue(&terminate_req_sem, &inf_sem_check) != 0)
            {
                goto err_getsem;
            }
            /*Checks The Semaphore Value*/
            if(inf_sem_check != 1)
            {
                goto ai_inf_end;
            }
            /*Wait Till The DRP-AI Ends*/
            FD_ZERO(&rfds0);
            FD_SET(drpai_fd0, &rfds0);
            tv.tv_sec = 0;
            tv.tv_nsec = DRPAI_TIMEOUT;
            ret_drpai = pselect(drpai_fd0+1, &rfds0, NULL, NULL, &tv, &sigset);
            if(ret_drpai == 0)
            {
                if(image_frame_ready != image_frame_used)
                {
                    image_frame_used = image_frame_ready;
                    /*Swap input buffer*/
                    if(0 == idata_swap_flag)
                    {
                        idata_idx = !idata_idx;
                        idata_swap_flag++;
                    }
                    /*Copy input image to expanded area*/
                    if(SUCCESS != get_input(mem_fd))
                    {
                        goto err_get_input;
                    }
                }
            }
            else if(ret_drpai < 0)
            {
                goto err_ai_inference;
            }
            else
            {
                /*Gets AI Inference End Time*/
                if(timespec_get(&inf_end_time, TIME_UTC) == 0)
                {
                    goto err_gettime_infend;
                }
                yolo_time = (uint32_t)((timedifference_msec(start_time, inf_end_time)*TIME_COEF));
                /*Checks If DRPAI Inference Ended Without Issue*/
                inf_status = ioctl(drpai_fd0, DRPAI_GET_STATUS, &drpai_status);
                if(inf_status == 0)
                {
                    /*Get DRPAI Output Data*/
                    if (ioctl(drpai_fd0, DRPAI_ASSIGN, &drpai_data0)!=0)
                    {
                        goto err_drpai_assign;
                    }
                    if(read(drpai_fd0, &drpai_output_buf0[0], drpai_data0.size) < 0)
                    {
                        goto err_drpai_read;
                    }
                    /*Preparation For Post-Processing*/
                    bcount = 0;
                    det_res.clear();
                    /*CPU Post-Processing For YOLOv3*/
                    R_Post_Proc(&drpai_output_buf0[0], det_res, &bcount);
                    /*Displays AI Inference Results & Processing Time On Console*/
                    print_boxes(det_res, bcount);
                    /*Count the Number of People Detected*/
                    ppl_count_local = 0;
                    people_counter(det_res, det_ppl, bcount, &ppl_count_local);
                    /*If Person is detected run HRNet for Pose Estimation three times*/
                    if(ppl_count_local > 0)
                    {
                        for(uint8_t i = 0; i < ppl_count_local; i++)
                        {
                            croph[i] = det_ppl[i].bbox.h + CROP_ADJ_X;
                            cropw[i] = det_ppl[i].bbox.w + CROP_ADJ_Y;
                            output_w[i] = cropw[i]*DISPLAY_WIDTH/IMREAD_IMAGE_WIDTH;
                            output_h[i] = croph[i]*DISPLAY_HEIGHT/IMREAD_IMAGE_HEIGHT;
                            /*Checks that cropping height and width does not exceeds image dimension*/
                            if(croph[i] < 1)
                            {
                                croph[i] = 1;
                                output_h[i] = DISPLAY_HEIGHT/IMREAD_IMAGE_HEIGHT;
                            }
                            else if(croph[i] > IMREAD_IMAGE_HEIGHT)
                            {
                                croph[i] = IMREAD_IMAGE_HEIGHT;
                                output_h[i] = DISPLAY_HEIGHT;
                            }
                            else
                            {
                                /*Do Nothing*/
                            }
                            if(cropw[i] < 1)
                            {
                                cropw[i] = 1;
                                output_w[i] = DISPLAY_WIDTH/IMREAD_IMAGE_WIDTH;
                            }
                            else if(cropw[i] > IMREAD_IMAGE_WIDTH)
                            {
                                cropw[i] = IMREAD_IMAGE_WIDTH;
                                output_w[i] = DISPLAY_WIDTH;
                            }
                            else
                            {
                                /*Do Nothing*/
                            }
                            /*Compute Cropping Y Position based on Detection Result*/
                            /*If Negative Cropping Position*/
                            if(det_ppl[i].bbox.y < (croph[i]/2))
                            {
                                cropy[i] = 0;
                                output_top[i] = 0;
                            }
                            else if(det_ppl[i].bbox.y > (IMREAD_IMAGE_HEIGHT-croph[i]/2)) /*If Exceeds Image Area*/
                            {
                                cropy[i] = IMREAD_IMAGE_HEIGHT-croph[i];
                                output_top[i] = DISPLAY_HEIGHT-output_h[i];
                            }
                            else
                            {
                                cropy[i] = (int16_t)det_ppl[i].bbox.y - croph[i]/2;
                                output_top[i] = cropy[i]*DISPLAY_HEIGHT/IMREAD_IMAGE_HEIGHT;
                            }
                            /*Compute Cropping X Position based on Detection Result*/
                            /*If Negative Cropping Position*/
                            if(det_ppl[i].bbox.x < (cropw[i]/2))
                            {
                                cropx[i] = 0;
                                output_left[i] = 0;
                            }
                            else if(det_ppl[i].bbox.x > (IMREAD_IMAGE_WIDTH-cropw[i]/2)) /*If Exceeds Image Area*/
                            {
                                cropx[i] = IMREAD_IMAGE_WIDTH-cropw[i];
                                output_left[i] = DISPLAY_WIDTH-output_w[i];
                            }
                            else
                            {
                                cropx[i] = (int16_t)det_ppl[i].bbox.x - cropw[i]/2;
                                output_left[i] = cropx[i]*DISPLAY_WIDTH/IMREAD_IMAGE_WIDTH;
                            }
                            /*Checks that combined cropping position with width and height does not exceed the image dimension*/
                            if(cropx[i] + cropw[i] > IMREAD_IMAGE_WIDTH)
                            {
                                cropw[i] = IMREAD_IMAGE_WIDTH - cropx[i];
                                output_w[i] = cropw[i]*DISPLAY_WIDTH/IMREAD_IMAGE_WIDTH;
                            }
                            if(cropy[i] + croph[i] > IMREAD_IMAGE_HEIGHT)
                            {
                                croph[i] = IMREAD_IMAGE_HEIGHT - cropy[i];
                                output_h[i] = croph[i]*DISPLAY_HEIGHT/IMREAD_IMAGE_HEIGHT;
                            }
                            /*Change HRNet Crop Parameters*/
                            crop_param.img_owidth = (uint16_t)cropw[i];
                            crop_param.img_oheight = (uint16_t)croph[i];
                            crop_param.pos_x = (uint16_t)cropx[i];
                            crop_param.pos_y = (uint16_t)cropy[i];
                            crop_param.obj.address = proc[DRPAI_INDEX_DRP_PARAM].address;
                            crop_param.obj.size = proc[DRPAI_INDEX_DRP_PARAM].size;
                            if (ioctl(drpai_fd, DRPAI_PREPOST_CROP, &crop_param)!=0)
                            {
                                goto err_drpai_crop;
                            }
                            /*Get Inference Start Time*/
                            if(timespec_get(&start_time, TIME_UTC) == 0)
                            {
                                goto err_gettime_start;
                            }
                            /*Start DRP-AI Driver*/
                            if (SUCCESS != ioctl(drpai_fd, DRPAI_START, &proc[0]))
                            {
                                goto err_start_inf;
                            }
                            if (i >= ppl_count_local - 1)
                            {
                                if(image_frame_ready != image_frame_used)
                                {
                                    image_frame_used = image_frame_ready;
                                    /*Swap input buffer*/
                                    if(0 == idata_swap_flag)
                                    {
                                        idata_idx = !idata_idx;
                                        idata_swap_flag++;
                                    }
                                    /*Copy input image to expanded area*/
                                    if(SUCCESS != get_input(mem_fd))
                                    {
                                        goto err_get_input;
                                    }
                                }
                            }
                            while(1)
                            {
                                /*Gets The Termination Request Semaphore Value, If Different Then 1 Termination Was Requested*/
                                /*Checks If sem_getvalue Is Executed Without Issue*/
                                if(sem_getvalue(&terminate_req_sem, &inf_sem_check) != 0)
                                {
                                    goto err_getsem;
                                }
                                /*Checks The Semaphore Value*/
                                if(inf_sem_check != 1)
                                {
                                    goto ai_inf_end;
                                }
                                /*Wait Till The DRP-AI Ends*/
                                FD_ZERO(&rfds);
                                FD_SET(drpai_fd, &rfds);
                                tv.tv_sec = 0;
                                tv.tv_nsec = DRPAI_TIMEOUT;
                                ret_drpai = pselect(drpai_fd+1, &rfds, NULL, NULL, &tv, &sigset);
                                if(ret_drpai == 0)
                                {
                                    /*Do Nothing*/
                                }
                                else if(ret_drpai < 0)
                                {
                                    goto err_ai_inference;
                                }
                                else
                                {
                                    /*Gets AI Inference End Time*/
                                    if(timespec_get(&inf_end_time, TIME_UTC) == 0)
                                    {
                                        goto err_gettime_infend;
                                    }
                                    hrnet_time[i] = (uint32_t)((timedifference_msec(start_time, inf_end_time)*TIME_COEF));
                                    /*Checks If DRPAI Inference Ended Without Issue*/
                                    inf_status = ioctl(drpai_fd, DRPAI_GET_STATUS, &drpai_status);
                                    if(inf_status == 0)
                                    {
                                        /*Get DRPAI Output Data*/
                                        if (ioctl(drpai_fd, DRPAI_ASSIGN, &drpai_data)!=0)
                                        {
                                            goto err_drpai_assign;
                                        }
                                        if(read(drpai_fd, &drpai_output_buf[0], drpai_data.size) < 0)
                                        {
                                            goto err_drpai_read;
                                        }
                                        /*CPU Post Processing For HRNet & Display the Results*/
                                        R_Post_Proc_HRNet(&drpai_output_buf[0],i);
                                        if(lowest_kpt_score_local[i] > 0)
                                        {
                                            R_HRNet_Coord_Convert(i);
                                        }
                                        break;
                                    }
                                    else //inf_status != 0
                                    {
                                        goto err_drpai_status;
                                    }
                                }
                            }
                        }
                    }
                    /*Copy data for Display Thread*/
                    ppl_count = 0;
                    memcpy(lowest_kpt_score,lowest_kpt_score_local,sizeof(lowest_kpt_score_local));
                    memcpy(id_x, id_x_local, sizeof(id_x_local));
                    memcpy(id_y, id_y_local,sizeof(id_y_local));
                    ppl_count = ppl_count_local;
                    /*Display Processing Time On Console*/
                    ai_time = yolo_time;
                    for(uint8_t i = 0; i < ppl_count_local; i++)
                    {
                        ai_time += hrnet_time[i];
                    }
                    printf("Total AI Inference Time %d [ms]\n", ai_time);
                    break;
                }
                else //inf_status != 0
                {
                    goto err_drpai_status;
                }
            }
        }
    }
    /*End Of Inference Loop*/

/*Error Processing*/
err_open_mem_img:
    printf("\x1b[31;1m[ERROR] Failed to open /dev/mem for Image\n\x1b[0m");
    goto err;
err_load_drpai_param:
   printf("\x1b[31;1m[ERROR] Failed to load DRPAI Parameter\n\x1b[0m");
   goto err;
err_getsem:
    printf("\x1b[31;1m[ERROR] Failed to Get Semaphore Value\n\x1b[0m");
    goto err;
err_cb_timeout:
    printf("\x1b[31;1m[ERROR] B_IMAGE_StartVideoCapture Callback Timeout\n\x1b[0m");
    goto err;
err_gettime_start:
    printf("\x1b[31;1m[ERROR] Failed to Get Inference Start Time\n\x1b[0m");
    goto err;
err_start_inf:
    printf("\x1b[31;1m[ERROR] Failed to Start DRPAI Inference: %d\n\x1b[0m", errno);
    goto err;
err_ai_inference:
    printf("\x1b[31;1m[ERROR] DRPAI Inference pselect() Error: %d\n\x1b[0m", errno);
    goto err;
err_drpai_status:
    printf("\x1b[31;1m[ERROR] DRPAI Internal Error: %d\n\x1b[0m", errno);
    goto err;
err_gettime_infend:
    printf("\x1b[31;1m[ERROR] Failed to Get Inference End Time\n\x1b[0m");
    goto err;
err_drpai_assign:
    printf("\x1b[31;1m[ERROR] Failed to Assign DRPAI data: %d\n\x1b[0m", errno);
    goto err;
err_drpai_read:
    printf("\x1b[31;1m[ERROR] Failed to read DRPAI output data: %d\n\x1b[0m", errno);
    goto err;
err_drpai_crop:
    printf("\x1b[31;1m[ERROR] Failed to DRPAI prepost crop: %d\n\x1b[0m", errno);
    goto err;
err_get_input:
    printf("\x1b[31;1m[ERROR] Failed to get DRPAI input data\n\x1b[0m");
    goto err;

err:
    /*Set Termination Request Semaphore To 0*/
    sem_trywait(&terminate_req_sem);
    goto ai_inf_end;

ai_inf_end:
    if (r_image_get)
    {
        /*Release Image Object*/
        B_OBJ_EndReadingData(image_obj.handle);
        r_image_get = false;
    }
    printf("AI Inference Thread Terminated\n");
    pthread_exit(NULL);
}

static int16_t x_center = 0;
static int16_t y_center = 0;
static int16_t x_left = 0;
static int16_t y_top = 0;
static int16_t w_rect = 0;
static int16_t h_rect = 0;
/*****************************************
* Function Name : R_Display_Thread
* Description   : Executes the HDMI Display with Display thread
* Arguments     : threadid = thread identification
* Return value  : -
******************************************/
void *R_Display_Thread(void *threadid)
{
    /*Semaphore Variable*/
    int hdmi_sem_check;
    /*Set Image Info For Imshow*/
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
    /*First Loop Flag*/
    bool setting_loop = true;
    /*Skeleton Data For Drawing*/
    uint16_t skel_upper_x[11];
    uint16_t skel_upper_y[11];
    uint16_t skel_lower_x[8];
    uint16_t skel_lower_y[8];

    printf("Display Thread Starting\n");

    while(1)
    {
        /*Image Show Flag Monitor*/
        while(1)
        {
            /*Gets The Termination Request Semaphore Value, If Different Then 1 Termination Is Requested*/
            /*Checks If sem_getvalue Is Executed Without Issue*/
            if(sem_getvalue(&terminate_req_sem, &hdmi_sem_check) != 0)
            {
                 goto err_getsem;
            }
            /*Checks The Semaphore Value*/
            if(hdmi_sem_check != 1)
            {
                goto hdmi_end;
            }
            /*Checks If Image Frame for Imshow Is ready*/
            if(image_show_ready == image_show_used)
            {
                usleep(WAIT_TIME); //wait 1 tick time
            }
            else
            {
                image_show_used = image_show_ready;
                break;
            }
        }

        /*Graphic Display*/
        /*Get Image Object for HDMI Display*/
        hdmi_isp_err = B_IMAGE_Imread(source_id,&image_info_disp,IMAGE_NUM,&image_obj_disp);
        if(hdmi_isp_err != 0)
        {
            goto err_imread;
        }
        usleep(WAIT_TIME); //wait 1 tick time

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
        if(setting_loop)
        {
            /*Set Number Of Layer For Graphic Draw*/
            hdmi_isp_err = B_GRAPH_SetLayerNum(LAYER_NUM, DEBUG_LAYER_NUM);
            if(hdmi_isp_err != 0)
            {
                goto err_layer_num;
            }
            /*Set Layer Resolution For The Display*/
            hdmi_isp_err = B_GRAPH_SetLayerResolution(D_GRAPH_RESOLUTION_960x540);
            if(hdmi_isp_err != 0)
            {
                goto err_layer_reso;
            }
            /*Set Layer Coordinate Resolution*/
            hdmi_isp_err = B_GRAPH_SetCoordinateReso(D_GRAPH_RESOLUTION_960x540);
            if(hdmi_isp_err != 0)
            {
                goto err_layer_coor;
            }
            usleep(WAIT_TIME); //wait 1 tick time
            /*The Layer Setting Is Only Done Once*/
            setting_loop = false;
        }
        else
        {
            /*Clear Graphic Drawing Layer*/
            hdmi_isp_err = B_GRAPH_Clear(D_GRAPH_NORMAL_LAYER_1);
            if(hdmi_isp_err != 0)
            {
                goto err_layer_clear;
            }
            usleep(WAIT_TIME); //wait 1 tick time
            /*Copy Latest HRNet Inference Time Info Text Into Buffer*/
            sprintf(ai_time_str, "Total AI Inference Time:%dms\n", ai_time);
            /*Set The Inference Time Info Text Size*/
            hdmi_isp_err = B_GRAPH_SetTextSize(D_GRAPH_NORMAL_LAYER_1,AI_TIME_SIZE_WIDTH,AI_TIME_SIZE_HEIGHT);
            if(hdmi_isp_err != 0)
            {
                goto err_text_size;
            }
            usleep(WAIT_TIME); //wait 1 tick time
            /*Set The Inference Time Info Text Color*/
            hdmi_isp_err = B_GRAPH_SetTextColor(D_GRAPH_NORMAL_LAYER_1,D_GRAPH_COLOR_RED);
            if(hdmi_isp_err != 0)
            {
                goto err_text_color;
            }
            usleep(WAIT_TIME); //wait 1 tick time
            /*Set The Inference Time Info Text Edge Color*/
            hdmi_isp_err = B_GRAPH_SetTextEdgeColor(D_GRAPH_NORMAL_LAYER_1,D_GRAPH_COLOR_RED);
            if(hdmi_isp_err != 0)
            {
                goto err_text_edge_color;
            }
            usleep(WAIT_TIME); //wait 1 tick time
            /*Set The Inference Time Info Text Position*/
            hdmi_isp_err = B_GRAPH_SetTextPosition(D_GRAPH_NORMAL_LAYER_1,AI_TIME_POS_X,AI_TIME_POS_Y);
            if(hdmi_isp_err != 0)
            {
                goto err_text_pos;
            }
            usleep(WAIT_TIME); //wait 1 tick time
            /*Draw The Inference Time Info Text*/
            hdmi_isp_err = B_GRAPH_DrawText(D_GRAPH_NORMAL_LAYER_1, (int8_t *)(ai_time_str));
            if(hdmi_isp_err != 0)
            {
                goto err_draw_text;
            }
            usleep(WAIT_TIME); //wait 1 tick time
            if(ppl_count > 0)
            {
                for(uint8_t i=0; i < ppl_count; i++)
                {
                    if (lowest_kpt_score[i] > TH_KPT)
                    {
                        /*Set Skeleton Data*/
                        /*For Upper Part Poly Line Drawn In This Order: ID10 > ID8 > ID6 > ID4 > ID2 > ID0 > ID1 > ID3 > ID5 > ID7 >ID9*/
                        skel_upper_x[ 0] = id_x[10][i]; skel_upper_x[ 1] = id_x[ 8][i];
                        skel_upper_y[ 0] = id_y[10][i]; skel_upper_y[ 1] = id_y[ 8][i];
                        skel_upper_x[ 2] = id_x[ 6][i]; skel_upper_x[ 3] = id_x[ 4][i];
                        skel_upper_y[ 2] = id_y[ 6][i]; skel_upper_y[ 3] = id_y[ 4][i];
                        skel_upper_x[ 4] = id_x[ 2][i]; skel_upper_x[ 5] = id_x[ 0][i];
                        skel_upper_y[ 4] = id_y[ 2][i]; skel_upper_y[ 5] = id_y[ 0][i];
                        skel_upper_x[ 6] = id_x[ 1][i]; skel_upper_x[ 7] = id_x[ 3][i];
                        skel_upper_y[ 6] = id_y[ 1][i]; skel_upper_y[ 7] = id_y[ 3][i];
                        skel_upper_x[ 8] = id_x[ 5][i]; skel_upper_x[ 9] = id_x[ 7][i];
                        skel_upper_y[ 8] = id_y[ 5][i]; skel_upper_y[ 9] = id_y[ 7][i];
                        skel_upper_x[10] = id_x[ 9][i];
                        skel_upper_y[10] = id_y[ 9][i];
                        /*For Lower Part Poly Line Drawn In This Order: ID16 > ID14 > ID12 > ID6 > ID5 > ID11 > ID13 > ID15*/
                        skel_lower_x[ 0] = id_x[16][i]; skel_lower_x[ 1] = id_x[14][i];
                        skel_lower_y[ 0] = id_y[16][i]; skel_lower_y[ 1] = id_y[14][i];
                        skel_lower_x[ 2] = id_x[12][i]; skel_lower_x[ 3] = id_x[ 6][i];
                        skel_lower_y[ 2] = id_y[12][i]; skel_lower_y[ 3] = id_y[ 6][i];
                        skel_lower_x[ 4] = id_x[ 5][i]; skel_lower_x[ 5] = id_x[11][i];
                        skel_lower_y[ 4] = id_y[ 5][i]; skel_lower_y[ 5] = id_y[11][i];
                        skel_lower_x[ 6] = id_x[13][i]; skel_lower_x[ 7] = id_x[15][i];
                        skel_lower_y[ 6] = id_y[13][i]; skel_lower_y[ 7] = id_y[15][i];
                        /*Set Skeleton Color*/
                        hdmi_isp_err = B_GRAPH_SetShapeLineColor(D_GRAPH_NORMAL_LAYER_1,D_GRAPH_COLOR_BLUE);
                        if(hdmi_isp_err != 0)
                        {
                            goto err_set_line_color;
                        }
                        usleep(WAIT_TIME); //wait 1 tick time
                        /*Draw Upper Skeleton*/
                        hdmi_isp_err = B_GRAPH_DrawPolyLine(D_GRAPH_NORMAL_LAYER_1,skel_upper_x,skel_upper_y,11);
                        if (hdmi_isp_err != 0)
                        {
                            goto err_draw_skel_up;
                        }
                        usleep(WAIT_TIME); //wait 1 tick time
                        /*Draw Lower Skeleton*/
                        hdmi_isp_err = B_GRAPH_DrawPolyLine(D_GRAPH_NORMAL_LAYER_1,skel_lower_x,skel_lower_y,8);
                        if (hdmi_isp_err != 0)
                        {
                            goto err_draw_skel_low;
                        }
                        usleep(WAIT_TIME); //wait 1 tick time
                        /*Draw Skeleton Leg Connection*/
                        hdmi_isp_err = B_GRAPH_DrawLine(D_GRAPH_NORMAL_LAYER_1, id_x[11][i], id_y[11][i], id_x[12][i], id_y[12][i]);
                        if (hdmi_isp_err != 0)
                        {
                            goto err_draw_line;
                        }
                        usleep(WAIT_TIME); //wait 1 tick time
                        /*Draw Rectangle As Key Points*/
                        hdmi_isp_err = B_GRAPH_SetShapeLineColor(D_GRAPH_NORMAL_LAYER_1,D_GRAPH_COLOR_RED);
                        if(hdmi_isp_err != 0)
                        {
                            goto err_set_line_color;
                        }
                        usleep(WAIT_TIME); //wait 1 tick time
                        for(uint8_t v = 0; v < NUM_OUTPUT_C; v++)
                        {
                            /*Draw Rectangles On Each Skeleton Key Points*/
                            hdmi_isp_err = B_GRAPH_DrawRect(D_GRAPH_NORMAL_LAYER_1,id_x[v][i],id_y[v][i],KEY_POINT_SIZE,KEY_POINT_SIZE);
                            if(hdmi_isp_err != 0)
                            {
                                goto err_draw_keypoints;
                            }
                            usleep(WAIT_TIME); //wait 1 tick time
                        }
                    }
                }
            }
            /*Execution Of Graphic Display*/
            hdmi_isp_err = B_GRAPH_ExeDisplay(D_GRAPH_NORMAL_LAYER_1);
            if(hdmi_isp_err != 0)
            {
                goto err_exe_disp;
            }
            usleep(WAIT_TIME); //wait 1 tick time
        }
        /*Display On HDMI Monitor*/
        hdmi_isp_err = B_IMAGE_Imshow(&image_obj_disp_cp,&disp_info);
        if(hdmi_isp_err == ERR_SYSTEM_ID)
        {
            printf("[INFO] B_IMAGE_Imshow Failed: %d\n", hdmi_isp_err);
        }
        else if(hdmi_isp_err != 0)
        {
            goto err_imshow;
        }
        usleep(WAIT_TIME); //wait 1 tick time
        hdmi_isp_err = B_OBJ_FreeObject(image_obj_disp_cp.handle);
        if (0 != hdmi_isp_err)
        {
            fprintf(stderr, "[ERROR] Failed to run B_OBJ_FreeObject copied display image object: %d\n", hdmi_isp_err);
            goto err;
        }
    } /*End Of Loop*/

/*Error Processing*/
err_getsem:
    printf("\x1b[31;1m[ERROR] Failed to Get Semaphore Value\n\x1b[0m");
    goto err;
err_imread:
    printf("\x1b[31;1m[ERROR] B_IMAGE_Imread(Display Thread) Failed: %d\n\x1b[0m", hdmi_isp_err);
    goto err;
err_layer_num:
    printf("\x1b[31;1m[ERROR] B_GRAPH_SetLayerNum Failed: %d\n\x1b[0m", hdmi_isp_err);
    goto err;
err_layer_reso:
    printf("\x1b[31;1m[ERROR] B_GRAPH_SetLayerResolution Failed: %d\n\x1b[0m", hdmi_isp_err);
    goto err;
err_layer_coor:
    printf("\x1b[31;1m[ERROR] B_GRAPH_SetCoordinateReso Failed: %d\n\x1b[0m", hdmi_isp_err);
    goto err;
err_layer_clear:
    printf("\x1b[31;1m[ERROR] B_GRAPH_Clear Failed: %d\n\x1b[0m", hdmi_isp_err);
    goto err;
err_text_color:
    printf("\x1b[31;1m[ERROR] B_GRAPH_SetTextColor Failed: %d\n\x1b[0m", hdmi_isp_err);
    goto err;
err_text_edge_color:
    printf("\x1b[31;1m[ERROR] B_GRAPH_SetTextEdgeColor Failed: %d\n\x1b[0m", hdmi_isp_err);
    goto err;
err_text_size:
    printf("\x1b[31;1m[ERROR] B_GRAPH_SetTextSize Failed: %d\n\x1b[0m", hdmi_isp_err);
    goto err;
err_text_pos:
    printf("\x1b[31;1m[ERROR] B_GRAPH_SetTextPosition Failed: %d\n\x1b[0m", hdmi_isp_err);
    goto err;
err_draw_text:
    printf("\x1b[31;1m[ERROR] B_GRAPH_DrawText Failed: %d\n\x1b[0m", hdmi_isp_err);
    goto err;
err_set_line_color:
    printf("\x1b[31;1m[ERROR] B_GRAPH_SetShapeLineColor Failed: %d\n\x1b[0m", hdmi_isp_err);
    goto err;
err_draw_rect:
    printf("\x1b[31;1m[ERROR] B_GRAPH_DrawRect Failed: %d\n\x1b[0m", hdmi_isp_err);
    goto err;
err_draw_skel_up:
    printf("\x1b[31;1m[ERROR] B_GRAPH_DrawPolyLine Failed (Upper Skeleton): %d\n\x1b[0m", hdmi_isp_err);
    goto err;
err_draw_skel_low:
    printf("\x1b[31;1m[ERROR] B_GRAPH_DrawPolyLine Failed (Lower Skeleton): %d\n\x1b[0m", hdmi_isp_err);
    goto err;
err_draw_line:
    printf("\x1b[31;1m[ERROR] B_GRAPH_DrawLine Failed (Lower Skeleton): %d\n\x1b[0m", hdmi_isp_err);
    goto err;
err_draw_keypoints:
    printf("\x1b[31;1m[ERROR] B_GRAPH_DrawRect Failed (Key Points): %d\n\x1b[0m", hdmi_isp_err);
    goto err;
err_exe_disp:
    printf("\x1b[31;1m[ERROR] B_GRAPH_ExeDisplay Failed: %d\n\x1b[0m", hdmi_isp_err);
    goto err;
err_imshow:
    printf("\x1b[31;1m[ERROR] B_IMAGE_Imshow Failed: %d\n\x1b[0m", hdmi_isp_err);
    goto err;

err:
    /*Set Termination Request Semaphore To 0*/
    sem_trywait(&terminate_req_sem);
    goto hdmi_end;

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
    int ret_fcntl;
    int semget_err;
    int semget_val;

    printf("Key Hit Thread Started\n");
    /*Set Standard Input To Non Blocking*/
    ret_fcntl = fcntl(0, F_SETFL, O_NONBLOCK);
    if(ret_fcntl == -1)
    {
        goto err_fcntl;
    }
    while(1)
    {
        /*Checks Termination Request Semaphore Value*/
        semget_err = sem_getvalue(&terminate_req_sem, &semget_val);
        if(semget_err != 0)
        {
            goto err_semget;
        }
        if(semget_val != 1) /*Termination Request Detected On Other Threads*/
        {
            goto key_hit_end;
        }
        if(getchar() != EOF) /*Application Stop Key (Enter) Detected*/
        {
            printf("Enter Detected\n");
            goto err;
        }
        else
        {
            usleep(WAIT_TIME);
        }
    }
/*Error Processing*/
err_fcntl:
    printf("\x1b[31;1m[ERROR] Failed to fctnl() %d\n\x1b[0m", errno);
    goto err;
err_semget:
    printf("\x1b[31;1m[ERROR] Failed to Get Semaphore Value %d\n\x1b[0m", errno);
    goto err;
err:
    /*Set Termination Request Semaphore to 0*/
    sem_trywait(&terminate_req_sem);
    goto key_hit_end;
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
uint8_t R_Main_Process()
{
    /*Main Process Variables*/
    uint8_t main_ret = 0;
    /*Semaphore Related*/
    int sem_check;
    int semget_err;

    printf("Main Loop Starts\n");
    while(1)
    {
        /*Checks If There Is Any Termination Request In The Other Threads*/
        semget_err = sem_getvalue(&terminate_req_sem, &sem_check);
        if(semget_err != 0)
        {
            goto err_semget;
        }
        /*Checks The Termination Request Semaphore Value*/
        if(sem_check != 1)
        {
            goto main_proc_end;
        }
        usleep(WAIT_TIME); //wait 1 tick time
    } //end main loop
/*Error Processing*/
err_semget:
    printf("\x1b[31;1m[ERROR] Failed to get Semaphore Value\n\x1b[0m");
    goto err;

err:
    /*Set Termination Request Semaphore To 0*/
    sem_trywait(&terminate_req_sem);
    main_ret = 1;
    goto main_proc_end;

main_proc_end:
    goto end;

end:
    printf("Main Process Terminated\n");
    return main_ret;
}


int main(void)
{
    uint8_t main_proc;
    uint8_t ret;
    /*Multithreading Variables*/
    int create_thread_ai = -1;
    int create_thread_key = -1;
    int create_thread_hdmi = -1;
    int sem_create = -1;
    /*Core1 Command Controller Function Returns*/
    err_t isp_err = 0;
    printf("Sample Application Started\n");
    /*Termination Request Semaphore Initialization*/
    sem_create = sem_init(&terminate_req_sem,0,1); // thread shared semaphore, initialized value at 1
    if(sem_create != 0)
    {
        goto err_sem_init;
    }
    /*DRP-AI Driver Open*/
    /*For YOLOv3*/
    drpai_fd0 = open("/dev/drpai0", O_RDWR);
    if (drpai_fd0 < 0)
    {
        goto err_open_drpai;
    }
    /*For HRNet*/
    drpai_fd = open("/dev/drpai0", O_RDWR);
    if (drpai_fd < 0)
    {
        goto err_open_drpai;
    }
    /* YOLOv3 */
    prefix = AI0_DESC_NAME;
    dir = prefix + "/";
    address_file = dir + prefix + "_addrmap_intm.txt";
    ret = read_addrmap_txt(address_file, &drpai_address0);
    if (ret > SUCCESS)
    {
        goto err_read_addrmap;
    }
    ret = load_drpai_data(drpai_fd0, drpai_address0);
    if (ret != SUCCESS)
    {
        goto err_load_drpai;
    }
    /* HRNet */
    prefix = AI_DESC_NAME;
    dir = prefix + "/";
    address_file = dir + prefix + "_addrmap_intm.txt";
    ret = read_addrmap_txt(address_file, &drpai_address);
    if (ret > SUCCESS)
    {
        goto err_read_addrmap;
    }
    ret = load_drpai_data(drpai_fd, drpai_address);
    if (ret != SUCCESS)
    {
        goto err_load_drpai;
    }
    /*Frame Info*/
    frame_info.cfg  = D_IMAGE_SIZE_QHD_30P;
    frame_info.n_buffer = MIPI_BUFFER;
    /*Start Capturing Frames From MIPI Camera*/
    source_id = B_IMAGE_StartVideoCapture(&frame_info,image_frame_callback); // everytime a new frame is ready the callback will change frame_ready flag
    if(source_id == 0)
    {
        goto err_start_capt;
    }
    /*Create Key Hit Thread*/
    create_thread_key = pthread_create(&kbhit_thread, NULL, R_Kbhit_Thread, NULL);
    if(create_thread_key != 0)
    {
        goto err_keyhit_thread;
    }
    /*Create Inference Thread*/
    create_thread_ai = pthread_create(&ai_inf_thread, NULL, R_Inf_Thread, NULL);
    if(create_thread_ai != 0)
    {
        goto err_inf_thread;
    }
    /*Create Display Thread*/
    create_thread_hdmi = pthread_create(&hdmi_thread, NULL, R_Display_Thread, NULL);
    if(create_thread_hdmi != 0)
    {
        goto err_hdmi_thread;
    }
    /*Run Main Process*/
    main_proc = R_Main_Process();
    if(main_proc != 0)
    {
        goto err_main_proc;
    }
    goto main_end;
/*Error Processing*/
err_read_addrmap:
    printf("\x1b[31;1m[ERROR] Failed to Read Address Map Text File\n\x1b[0m");
    goto main_end;
err_open_drpai:
    printf("\x1b[31;1m[ERROR] Failed to Open DRP-AI Driver: %d\n\x1b[0m", errno);
    goto main_end;
err_load_drpai:
    printf("\x1b[31;1m[ERROR] Failed to load DRPAI Data\n\x1b[0m");
    goto main_end;
err_sem_init:
    printf("\x1b[31;1m[ERROR] Failed to Initialize Termination Request Semaphore\n\x1b[0m");
    goto main_end;
err_start_capt:
    printf("\x1b[31;1m[ERROR] B_IMAGE_StartVideoCapture Failed: %ld\n\x1b[0m", source_id);
    goto main_end;
err_inf_thread:
    sem_trywait(&terminate_req_sem);
    printf("\x1b[31;1m[ERROR] AI Thread Creation Failed\n\x1b[0m");
    goto main_end;
err_hdmi_thread:
    sem_trywait(&terminate_req_sem);
    printf("\x1b[31;1m[ERROR] Display Thread Creation Failed\n\x1b[0m");
    goto main_end;
err_keyhit_thread:
    printf("\x1b[31;1m[ERROR] Key Hit Thread Creation Failed\n\x1b[0m");
    goto main_end;
err_main_proc:
    printf("\x1b[31;1m[ERROR] Error during Main Process\n\x1b[0m");
    goto main_end;
/*Termination*/
main_end:
    /*Wait Till Display Thread Terminates*/
    if(create_thread_hdmi == 0)
    {
        if(wait_join(&hdmi_thread, AI_THREAD_TIMEOUT) != 0)
        {
            printf("\x1b[31;1m[ERROR] Display Thread Failed to Exit on time\n\x1b[0m");
        }
    }
    /*Wait Till AI Inference Thread Terminates*/
    if(create_thread_ai == 0)
    {
        if(wait_join(&ai_inf_thread, AI_THREAD_TIMEOUT) != 0)
        {
            printf("\x1b[31;1m[ERROR] Inference Thread Failed to Exit on time\n\x1b[0m");
        }
    }
    /*Wait Till Key Hit Thread Terminates*/
    if(create_thread_key == 0)
    {
        if(wait_join(&kbhit_thread, KEY_THREAD_TIMEOUT) != 0)
        {
            printf("\x1b[31;1m[ERROR] Key Hit Thread Failed to Exit on time\n\x1b[0m");
        }
    }
    /*Stop Video Capture*/
    if(source_id!=0)
    {
        isp_err = B_IMAGE_StopVideoCapture(source_id);
        if(isp_err !=0)
        {
            printf("\x1b[31;1m[ERROR] B_IMAGE_StopVideoCapture Failed: %d\n\x1b[0m",isp_err);
        }
    }
    /*DRP-AI Driver Close*/
    if(drpai_fd0 >= 0)
    {
        if(close(drpai_fd0) != 0)
        {
            printf("\x1b[31;1m[ERROR] Failed to Close DRP-AI Driver (YOLOv3): %d\n\x1b[0m", errno);
        }
    }
    if(drpai_fd >= 0)
    {
        if(close(drpai_fd) != 0)
        {
            printf("\x1b[31;1m[ERROR] Failed to Close DRP-AI Driver (HRNet): %d\n\x1b[0m", errno);
        }
    }
    /*Delete Terminate Request Semaphore*/
    if(sem_create == 0)
    {
        sem_destroy(&terminate_req_sem);
    }
    printf("Application End\n");
    return 0;
}
