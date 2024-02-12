/*-----------------------------------------------------------------------------------------------*/
/** @file	b_std.h
	@brief	BURST Standard header
    Copyright (C) 2022 CASIO COMPUTER CO., LTD.  All rights reserved.
    Copyright (C) 2022 Renesas Electronics Corporation. All rights reserved.
*/
/*-----------------------------------------------------------------------------------------------*/

#ifndef	__B_STD_H__
#define	__B_STD_H__

#include	<stdint.h>
#include	<stdbool.h>

typedef	unsigned int		bool_t;
typedef	int32_t				err_t;

#ifndef	NULL
#define	NULL				(0)
#endif

#define	B2_ERR_OK			(0)

#define	B2_WRN_SYSTEM_NOSPT	(-1)
#define	B2_WRN_SYSTEM_PAR	(-2)
#define	B2_WRN_SYSTEM_ID	(-3)
#define	B2_WRN_SYSTEM_NOID	(-4)
#define	B2_WRN_SYSTEM_CTX	(-5)
#define	B2_WRN_SYSTEM_OBJ	(-6)
#define	B2_WRN_SYSTEM_NOEXS	(-7)
#define	B2_WRN_SYSTEM_NOMEM	(-8)
#define	B2_WRN_SYSTEM_BOVR	(-9)
#define	B2_WRN_SYSTEM_QOVR	(-10)
#define	B2_WRN_SYSTEM_TMOUT	(-11)
#define	B2_WRN_SYSTEM_CAN	(-12)
#define	B2_WRN_SYSTEM_REF	(-13)
#define	B2_WRN_SYSTEM_DEV	(-14)
#define	B2_WRN_SYSTEM_DAT	(-15)
#define	B2_WRN_SYSTEM_STA	(-16)
#define	B2_WRN_SYSTEM_SEC	(-17)
#define	B2_WRN_SYSTEM_VER	(-18)

#define	B2_ERR_SYSTEM_NOSPT	(-201)
#define	B2_ERR_SYSTEM_PAR	(-202)
#define	B2_ERR_SYSTEM_ID	(-203)
#define	B2_ERR_SYSTEM_NOID	(-204)
#define	B2_ERR_SYSTEM_CTX	(-205)
#define	B2_ERR_SYSTEM_OBJ	(-206)
#define	B2_ERR_SYSTEM_NOEXS	(-207)
#define	B2_ERR_SYSTEM_NOMEM	(-208)
#define	B2_ERR_SYSTEM_BOVR	(-209)
#define	B2_ERR_SYSTEM_QOVR	(-210)
#define	B2_ERR_SYSTEM_TMOUT	(-211)
#define	B2_ERR_SYSTEM_CAN	(-212)
#define	B2_ERR_SYSTEM_REF	(-213)
#define	B2_ERR_SYSTEM_DEV	(-214)
#define	B2_ERR_SYSTEM_DAT	(-215)
#define	B2_ERR_SYSTEM_STA	(-216)
#define	B2_ERR_SYSTEM_SEC	(-217)
#define	B2_ERR_SYSTEM_VER	(-218)

#endif	// __B_STD_H__

/*-----------------------------------------------------------------------------------------------*/
/*  file end of "b_std.h"                                                                        */
/*-----------------------------------------------------------------------------------------------*/
