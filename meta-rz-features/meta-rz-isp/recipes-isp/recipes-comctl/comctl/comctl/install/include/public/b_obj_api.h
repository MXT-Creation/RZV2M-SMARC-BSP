/*-----------------------------------------------------------------------------------------------*/
/**
    @file   b_obj_api.h
    @brief  B2 Object Management API
    Copyright (C) 2022 CASIO COMPUTER CO., LTD.  All rights reserved.
    Copyright (C) 2022 Renesas Electronics Corporation. All rights reserved.
*/
/*-----------------------------------------------------------------------------------------------*/

#ifndef B_OBJ_API_H__
#define B_OBJ_API_H__

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct
    {
        uint16_t year;
        uint16_t month;
        uint16_t day;
        uint16_t hour;
        uint16_t minute;
        uint16_t second;
        uint16_t msec;
        uint16_t usec;
    } st_video_time_info_t;



err_t B_OBJ_FreeObject(uint64_t obj_handle);
void* B_OBJ_BeginReadingData(uint64_t obj_h);
err_t B_OBJ_EndReadingData(uint64_t obj_h);
err_t B_OBJ_GetTimeInfo(uint64_t obj_h, st_video_time_info_t *p_time_info);
void* B_OBJ_BeginWritingData(uint64_t obj_h);
err_t B_OBJ_EndWritingData(uint64_t obj_h);


#ifdef __cplusplus
}
#endif

#endif // #ifndef B_OBJ_API_H__

/*-----------------------------------------------------------------------------------------------*/
// file end of "b_obj_api.h"
/*-----------------------------------------------------------------------------------------------*/
