/*-----------------------------------------------------------------------------------------------*/
/**
    @file	b_image_api.h
    @brief	B2SDK Public header file for shooting
    Copyright (C) 2022 CASIO COMPUTER CO., LTD.  All rights reserved.
    Copyright (C) 2022 Renesas Electronics Corporation. All rights reserved.
*/
/*-----------------------------------------------------------------------------------------------*/

#ifndef B_IMAGE_MOVIE_PLAY_API_H__
#define B_IMAGE_MOVIE_PLAY_API_H__

/* Include component file. */
#include <b_image_api.h>
#include <b_image_movie_rec_api.h>

#ifdef __cplusplus
extern "C" {
#endif


#define D_IMAGE_MOVIE_PLAY_OUTPUT_MODE_YUYV		(0) 		///< YUYV
#define D_IMAGE_MOVIE_PLAY_OUTPUT_MODE_YUV420	(1) 		///< YUV420

typedef void (*f_image_movie_play_dec_stream_callback_t)(err_t err);

typedef struct {
	uint32_t	width;
	uint32_t	height;
	uint32_t	codec;
	uint8_t		num_gop_frames;
	uint8_t		output_mode;
} st_image_movie_play_dec_stream_info_t;

err_t B_IMAGE_StartDecodeStream(st_image_movie_play_dec_stream_info_t *p_dec_strm_info);
err_t B_IMAGE_SetDecodeStream(uint64_t input_addr, uint32_t input_size, f_image_movie_play_dec_stream_callback_t f_callback);
err_t B_IMAGE_DecodeStream(uint64_t output_addr, uint32_t output_size, f_image_movie_play_dec_stream_callback_t f_callback);
err_t B_IMAGE_StopDecodeStream(void);

#ifdef __cplusplus
}
#endif

#endif // #ifndef B_IMAGE_MOVIE_PLAY_API_H__

/*-----------------------------------------------------------------------------------------------*/
//	file end of "b_image_movie_play_api.h"
/*-----------------------------------------------------------------------------------------------*/
