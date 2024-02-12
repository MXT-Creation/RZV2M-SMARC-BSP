/*******************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products. No other uses are authorized. This
* software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT
* LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
* AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR
* ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE
* BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software
* and to discontinue the availability of this software. By using this software,
* you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
* Copyright (C) 2022 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************/ 
#ifndef __COMCTL_IF_H__
#define __COMCTL_IF_H__

#include "openamp/open_amp.h"
#include "b2sdk_if.h"

#if defined __cplusplus
extern "C" {
#endif

#define R_COMCTL_SOUND

#define R_COMCTL_OK                     ( 0L) // Success
/*B2SDK Special Error Code */
#define R_COMCTL_ERR_SEND               (-1L) // Send error(Do not use B2SDK)
#define R_COMCTL_ERR_TIMEOUT            (-2L) // Timeout error
#define R_COMCTL_ERR_PROC               (-3L) // Failed to create proc
#define R_COMCTL_ERR_RESOURCE_TABLE     (-4L) // Failed to create hil proc
#define R_COMCTL_ERR_MALLOC             (-5L) // Memory allocation failed
#define R_COMCTL_ERR_INIT_REMOTEPROC    (-6L) // Failed to initialize remoteproc resource
#define R_COMCTL_ERR_DEINIT_REMOTEPROC  (-7L) // Failed to deinitialize remoteproc resource
#define R_COMCTL_ERR_PARAM              (-8L) // Incorrect Argument
#define R_COMCTL_ERR_CALLBACK           (-9L) // Unable to register/to get callback
#define R_COMCTL_ERR_B2SDK_BUSY         (-10L)// B2SDK cannot execute
#define R_COMCTL_ERR_SHAREMEM           (-11L)// Share memory error
#define R_COMCTL_ERR_TID_TABLE          (-12L)// Transaction ID table error (full table)
#define R_COMCTL_ERR_INTERNAL           (-13L)// Internal Error
#define R_COMCTL_ERR_CALLBACK_FULL      (-14L)// the table is full cannot register a new element
#define R_COMCTL_ERR_FOUND              (-15L)// Cannot found the request element
#define R_COMCTL_ERR_DEVMEM             (-16L)// failed open /dev/mem
#define R_COMCTL_ERR_MMAP               (-17L)// failed mmap
#define R_COMCTL_ERR_MUTEX              (-18L)// failed executing mutex related function
#define R_COMCTL_ERR_QUEUE_TX           (-19L)// Failed to send with IPC
#define R_COMCTL_ERR_QUEUE_RX           (-20L)// Failed to receive with IPC
#define R_COMCTL_ERR_QUEUE_OPEN         (-21L)// Failed to open the queue
#define R_COMCTL_ERR_SHM                (-22L)// Failed to shared memory
#define R_COMCTL_ERR_SIGACT             (-23L)// Failed to sigaction func
    
typedef struct command_payload
{
    uint16_t size;          //2bytes
    uint8_t com_type;       //1bytes
    uint8_t com_order;      //1bytes
    uint32_t com_id;        //4bytes
    uint32_t trans_id;      //4bytes
    uint16_t cb_type;       //2bytes
    uint16_t time_id;       //2bytes
    uint32_t time_stamp;    //4bytes
    uint32_t data[];        //4bytes each element
} r_comctl_payload_st;

enum poll_block_type
{
    R_COMCTL_POLL_BLOCK,
    R_COMCTL_POLL_NONBLOCK,
    R_COMCTL_POLL_MAX
};

/* R_COMCTL Function */
int64_t B_R_IMAGE_StartVideoCapture(st_video_frame_info_t* p_input_video_info, f_image_callback_t f_callback, err_t* perr);
err_t R_COMCTL_Init(void);
err_t R_COMCTL_Deinit(void);
err_t R_COMCTL_Poll(int32_t non_block);
#if defined __cplusplus
}
#endif

#endif /*__COMCTL_IF_H__*/
