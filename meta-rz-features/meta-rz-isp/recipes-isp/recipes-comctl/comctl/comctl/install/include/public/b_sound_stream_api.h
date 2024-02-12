/*-----------------------------------------------------------------------------------------------*/
/**
	@file	b_sound_stream_api.h
	@brief	sound stream public header
	Copyright (C) 2022 CASIO COMPUTER CO., LTD.  All rights reserved.
	Copyright (C) 2022 Renesas Electronics Corporation. All rights reserved.
*/
/*-----------------------------------------------------------------------------------------------*/

#ifndef __B_SOUND_STREAM_API_H__
#define __B_SOUND_STREAM_API_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define		D_SOUND_STREAM_CHANNEL_MONO		(1)		///< monaural
#define		D_SOUND_STREAM_CHANNEL_STEREO	(2)		///< stereo

#define		D_SOUND_STREAM_DEPTH_8		(1)		///<  8Bit
#define		D_SOUND_STREAM_DEPTH_16		(2)		///<  16Bits

#define		D_SOUND_STREAM_SAMPLE_RATE_48000		(0)		///< 48000Hz
#define		D_SOUND_STREAM_SAMPLE_RATE_44100		(1)		///< 44100Hz
#define		D_SOUND_STREAM_SAMPLE_RATE_32000		(2)		///< 32000Hz
#define		D_SOUND_STREAM_SAMPLE_RATE_24000		(3)		///< 24000Hz
#define		D_SOUND_STREAM_SAMPLE_RATE_22050		(4)		///< 22050Hz
#define		D_SOUND_STREAM_SAMPLE_RATE_16000		(5)		///< 16000Hz
#define		D_SOUND_STREAM_SAMPLE_RATE_12000		(6)		///< 12000Hz
#define		D_SOUND_STREAM_SAMPLE_RATE_11025		(7)		///< 11025Hz
#define		D_SOUND_STREAM_SAMPLE_RATE_8000			(8)		///< 8000Hz


typedef struct
{
	uint64_t			handle;			 ///< audio handle
	uint64_t			block_num;		 ///< audio block no
} st_sound_object_t;

typedef struct
{
	uint8_t channel;
	uint8_t depth;
	uint8_t sample_rate;
} st_sound_param_t;

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
} st_sound_time_info;

typedef struct
{
	uint64_t				addr;
	uint64_t				block_num;
	uint32_t				block_size;
	st_sound_time_info		sound_time_info;
} st_sound_stream_info_t;

typedef void (*f_sound_callback_t )(err_t err, st_sound_object_t *p_sound_obj);

err_t B_SOUND_StartAudioStream(st_sound_param_t *p_sound_param, f_sound_callback_t f_sound_callback);
err_t B_SOUND_StopAudioStream(void);
err_t B_SOUND_GetStreamAudioInfo(st_sound_object_t *p_sound_obj, st_sound_stream_info_t *p_sound_stream_info);


#ifdef __cplusplus
}
#endif


#endif // __B_SOUND_STREAM_API_H__

/*-----------------------------------------------------------------------------------------------*/
//	file end of "b_sound_stream_api.h"
/*-----------------------------------------------------------------------------------------------*/
