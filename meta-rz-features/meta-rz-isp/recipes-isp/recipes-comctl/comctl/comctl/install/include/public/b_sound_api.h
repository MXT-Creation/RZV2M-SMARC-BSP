/*-----------------------------------------------------------------------------------------------*/
/**
	@file	b_sound_api.h
	@brief	sound public header
	Copyright (C) 2022 CASIO COMPUTER CO., LTD.  All rights reserved.
	Copyright (C) 2022 Renesas Electronics Corporation. All rights reserved.
*/
/*-----------------------------------------------------------------------------------------------*/

#ifndef	__B_SOUND_API_H__
#define	__B_SOUND_API_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define D_SOUND_RECORD_LEVEL_LOW	(0x100)
#define D_SOUND_RECORD_LEVEL_HIGH	(0x101)

err_t B_SOUND_SetRecordLevel(uint16_t record_level);


#ifdef __cplusplus
}
#endif

#endif // __B_SOUND_API_H__

/*-----------------------------------------------------------------------------------------------*/
//	file end of "b_sound_api.h"
/*-----------------------------------------------------------------------------------------------*/
