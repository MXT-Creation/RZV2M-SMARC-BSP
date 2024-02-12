/*-----------------------------------------------------------------------------------------------*/
/**
	@file	b_sys_api.h
	@brief	B2SDK public header for system-related APIs
    Copyright (C) 2022 CASIO COMPUTER CO., LTD.  All rights reserved.
    Copyright (C) 2022 Renesas Electronics Corporation. All rights reserved.
*/
/*-----------------------------------------------------------------------------------------------*/

#ifndef	__B_SYS_API_H__
#define	__B_SYS_API_H__

#ifdef	__cplusplus
extern "C"
{
#endif

#define	D_SYS_SENSOR_IMX415				(1)			///< IMX415
#define	D_SYS_SENSOR_IMX462				(2)			///< IMX462
#define	D_SYS_SENSOR_AR1335				(3)			///< AR1335

#define	D_SYS_TIMELIMIT_OFF				(1)
#define	D_SYS_TIMELIMIT_ON				(2)

#define	D_SYS_DDR_2G					(1)			///< DDR 2G
#define	D_SYS_DDR_4G					(2)			///< DDR 4G

#define	D_SYS_AUDIO_AK					(1)			///< AK
#define	D_SYS_AUDIO_TI					(2)			///< TI

#define	D_SYS_TIME_SUMMER_TIME_ON		(0)			///< ON
#define	D_SYS_TIME_SUMMER_TIME_OFF		(1)			///< OFF


typedef struct
{
	uint32_t version;
	uint32_t sensor;
	uint32_t reserved1;
	uint32_t reserved2;
	uint32_t reserved3;
	uint32_t reserved4;
	uint32_t reserved5;
	uint32_t reserved6;
} st_embedded_info_t;

typedef struct
{
	uint8_t rtc_sec;
	uint8_t rtc_min;
	uint8_t rtc_hour;
	uint8_t rtc_mday;
	uint8_t rtc_mon;
	uint16_t rtc_year;
	uint8_t rtc_wday;
	uint16_t rtc_yday;
	uint8_t rtc_isdst;
} st_rtc_info_t;

typedef	void (*f_sys_timer_callback_t)(err_t err);


err_t B_SYS_GetEmbeddedInfo(st_embedded_info_t *p_embedded_info);
err_t B_SYS_ISPOpen(void);
err_t B_SYS_ISPClose(void);
err_t B_SYS_GetTime(int8_t hour, uint8_t minute, st_rtc_info_t *p_rtc_param);
err_t B_SYS_SetTime(int8_t hour, uint8_t minute, st_rtc_info_t *p_rtc_param);
bool_t B_SYS_GetDDRManualRetrainingMode(void);
err_t B_SYS_GetChipTemperature(int16_t *p_temperature_tsu0, int16_t *p_temperature_tsu1);
err_t B_SYS_GetChipTemperatureMeasurementTime(uint32_t *p_measurement_time);


#ifdef __cplusplus
}
#endif

#endif // __B_SYS_API_H__

/*-----------------------------------------------------------------------------------------------*/
//	file end of "b_sys_api.h"
/*-----------------------------------------------------------------------------------------------*/
