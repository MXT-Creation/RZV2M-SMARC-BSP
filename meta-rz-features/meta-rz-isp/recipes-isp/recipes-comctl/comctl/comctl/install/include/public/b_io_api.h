/*-----------------------------------------------------------------------------------------------*/
/**
    @file   b_io_api.h
    @brief	B2SDK Public header file for shooting
    Copyright (C) 2022 CASIO COMPUTER CO., LTD.  All rights reserved.
    Copyright (C) 2022 Renesas Electronics Corporation. All rights reserved.
*/
/*-----------------------------------------------------------------------------------------------*/

#ifndef __B_IO_API_H__
#define __B_IO_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#define		D_IO_AD_0						(0)		///< AD0
#define		D_IO_AD_1						(1)		///< AD1

#define		D_IO_AD_CH00					(0)		///< AD Channel 0
#define		D_IO_AD_CH01					(1)		///< AD Channel 1
#define		D_IO_AD_CH02					(2)		///< AD Channel 2
#define		D_IO_AD_CH03					(3)		///< AD Channel 3
#define		D_IO_AD_CH04					(4)		///< AD Channel 4
#define		D_IO_AD_CH05					(5)		///< AD Channel 5
#define		D_IO_AD_CH06					(6)		///< AD Channel 6
#define		D_IO_AD_CH07					(7)		///< AD Channel 7
#define		D_IO_AD_CH08					(8)		///< AD Channel 8
#define		D_IO_AD_CH09					(9)		///< AD Channel 9
#define		D_IO_AD_CH010					(10)	///< AD Channel 10
#define		D_IO_AD_CH011					(11)	///< AD Channel 11

#define		D_IO_AD_GET_OFF					(0)		///< AD Channel ON
#define		D_IO_AD_GET_ON					(1)		///< AD Channel OFF

typedef struct
{
	uint8_t ad_sel;
	uint8_t ad_ch[12];
} st_io_ad_param_multi_t;

typedef struct
{
	uint8_t num;
	uint16_t *p_data;
} st_io_ad_data_multi_t;

typedef void (*f_io_ad_multi_callback_t)(st_io_ad_data_multi_t *ad_data);
err_t B_IO_GetAdDataMulti(st_io_ad_param_multi_t *p_io_ad_param, st_io_ad_data_multi_t *p_ad_data, f_io_ad_multi_callback_t io_ad_multi_callback);

#ifdef __cplusplus
}
#endif

#endif // __B_IO_API_H__

/*-----------------------------------------------------------------------------------------------*/
//	file end of "b_io_api.h"
/*-----------------------------------------------------------------------------------------------*/
