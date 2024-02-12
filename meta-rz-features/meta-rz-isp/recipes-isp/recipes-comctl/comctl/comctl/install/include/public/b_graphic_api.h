/*-----------------------------------------------------------------------------------------------*/
/**
	@file	b_graphic_api.h
	@brief	Drawing system public header file
	Copyright (C) 2022 CASIO COMPUTER CO., LTD.  All rights reserved.
    Copyright (C) 2022 Renesas Electronics Corporation. All rights reserved.
*/
/*-----------------------------------------------------------------------------------------------*/
#include "./b_image_api.h"

#ifndef	__B_GRAPHIC_API_H__
#define	__B_GRAPHIC_API_H__

#ifdef __cplusplus
extern "C"
{
#endif


#define		D_GRAPH_LAYER_TYPE_NORMAL		(1)
#define		D_GRAPH_LAYER_TYPE_DEBUG		(2)


#define		D_GRAPH_NORMAL_LAYER_1			(1)
#define		D_GRAPH_NORMAL_LAYER_2			(2)
#define		D_GRAPH_NORMAL_LAYER_3			(3)
#define		D_GRAPH_NORMAL_LAYER_4			(4)
#define		D_GRAPH_BACK_DRAW_LAYER			(21)

#define		D_GRAPH_RESOLUTION_1280x720		(0)
#define		D_GRAPH_RESOLUTION_1920x1080	(1)
#define		D_GRAPH_RESOLUTION_3840x2160	(2)
#define		D_GRAPH_RESOLUTION_640x360		(10)
#define		D_GRAPH_RESOLUTION_960x540		(11)
#define		D_GRAPH_RESOLUTION_480x270		(12)
#define		D_GRAPH_RESOLUTION_320x180		(13)
#define		D_GRAPH_RESOLUTION_1216x1024	(30)

#define		D_GRAPH_FONTLAYER_SIZE_1		(1)
#define		D_GRAPH_FONTLAYER_SIZE_2		(2)
#define		D_GRAPH_FONTLAYER_SIZE_3		(3)
#define		D_GRAPH_FONTLAYER_SIZE_4		(4)
#define		D_GRAPH_FONTLAYER_SIZE_5		(5)
#define		D_GRAPH_FONTLAYER_SIZE_6		(6)

#define		D_GRAPH_FONTLAYER_COLOR_WHITE	(0)
#define		D_GRAPH_FONTLAYER_COLOR_YELLOW	(1)
#define		D_GRAPH_FONTLAYER_COLOR_CYAN	(2)
#define		D_GRAPH_FONTLAYER_COLOR_GREEN	(3)
#define		D_GRAPH_FONTLAYER_COLOR_PURPLE	(4)
#define		D_GRAPH_FONTLAYER_COLOR_RED		(5)
#define		D_GRAPH_FONTLAYER_COLOR_BLUE	(6)
#define		D_GRAPH_FONTLAYER_COLOR_BLACK	(7)
#define		D_GRAPH_FONTLAYER_COLOR_GRAY	(8)

#define		D_GRAPH_COLOR_WHITE			(0xFFFFFFFF)
#define		D_GRAPH_COLOR_YELLOW		(0xFFFF00FF)
#define		D_GRAPH_COLOR_CYAN			(0x00FFFFFF)
#define		D_GRAPH_COLOR_GREEN			(0x008000FF)
#define		D_GRAPH_COLOR_PURPLE		(0x800080FF)
#define		D_GRAPH_COLOR_RED			(0xFF0000FF)
#define		D_GRAPH_COLOR_BLUE			(0x0000FFFF)
#define		D_GRAPH_COLOR_BLACK			(0x000000FF)
#define		D_GRAPH_COLOR_GRAY			(0x808080FF)
#define		D_GRAPH_COLOR_TRANSPARENT	(0x00000000)

#define		D_GRAPH_IMAGE_SIZE_FHD		(0)
#define		D_GRAPH_IMAGE_SIZE_HD		(1)
#define		D_GRAPH_IMAGE_SIZE_VGA		(2)

#define		D_GRAPH_OUTPUT_HDMI			(0)
#define		D_GRAPH_OUTPUT_LCD			(1)
#define		D_GRAPH_OUTPUT_NONE			(99)

typedef struct
{
	uint16_t left_x;
	uint16_t top_y;
	uint16_t width;
	uint16_t height;
	uint16_t arc_width;
	uint16_t arc_heigh;
} st_graph_round_rect_info_t;


typedef struct
{
	uint16_t start_x;
	uint16_t start_y;
	uint16_t control_x;
	uint16_t control_y;
	uint16_t end_x;
	uint16_t end_y;
} st_graph_qbezier_info_t;


typedef struct
{
	uint16_t center_x;
	uint16_t center_y;
	uint16_t width;
	uint16_t height;
	uint16_t start_angle;
	uint16_t extent_angle;
} st_graph_arc_info_t;





err_t B_GRAPH_SetLayerNum(uint8_t normal_layer_num, uint8_t debug_layer_num);
err_t B_GRAPH_SetLayerResolution(uint8_t resolution);
err_t B_GRAPH_SetCoordinateReso(uint8_t resolution);
err_t B_GRAPH_Clear(uint8_t layer);
void B_GRAPH_ClearFontLayer(void);
err_t B_GRAPH_SetFontLayerTextSize(uint8_t width_size, uint8_t height_size);
err_t B_GRAPH_SetFontLayerTextColor(uint8_t color);
err_t B_GRAPH_SetFontLayerPosition(uint8_t pos_x, uint8_t pos_y);
err_t B_GRAPH_DrawFontLayerText(const int8_t *format, ...);
err_t B_GRAPH_ExeDisplay(uint8_t layer);
err_t B_GRAPH_StartBackDraw(uint16_t posx, uint16_t posy, uint16_t width, uint16_t height);
err_t B_GRAPH_StopBackDraw(void);
err_t B_GRAPH_SetTextSize(uint8_t layer, uint8_t width, uint8_t height);
err_t B_GRAPH_SetTextColor(uint8_t layer, uint32_t color);
err_t B_GRAPH_SetTextEdgeWidth(uint8_t layer, uint8_t width);
err_t B_GRAPH_SetTextEdgeColor(uint8_t layer, uint32_t color);
err_t B_GRAPH_SetTextPosition(uint8_t layer, uint16_t pos_x, uint16_t pos_y);
err_t B_GRAPH_DrawText(uint8_t layer, const int8_t *format, ...);
err_t B_GRAPH_SetGridMaxCharNum(uint8_t layer, uint16_t max_w_char, uint16_t max_h_char);
err_t B_GRAPH_SetGridPosition(uint8_t layer, uint16_t grid_x, uint16_t grid_y);
err_t B_GRAPH_DrawGridText(uint8_t layer, const int8_t *format, ...);
err_t B_GRAPH_SetShapeLineWidth(uint8_t layer, uint16_t width);
err_t B_GRAPH_SetShapeLineColor(uint8_t layer, uint32_t color);
err_t B_GRAPH_SetShapeFillColor(uint8_t layer, uint32_t color);
err_t B_GRAPH_DrawLine(uint8_t layer, uint16_t pos_x1, uint16_t pos_y1, uint16_t pos_x2, uint16_t pos_y2);
err_t B_GRAPH_DrawRect(uint8_t layer, uint16_t left_x, uint16_t top_y, uint16_t width, uint16_t height);
err_t B_GRAPH_DrawCircle(uint8_t layer, uint16_t center_x, uint16_t center_y, uint16_t radius);
err_t B_GRAPH_DrawEllipse(uint8_t layer, uint16_t center_x, uint16_t center_y, uint16_t width, uint16_t height);
err_t B_GRAPH_DrawRoundRect(uint8_t layer, st_graph_round_rect_info_t *round_rect_info);
err_t B_GRAPH_DrawPolyLine(uint8_t layer, uint16_t *p_point_x, uint16_t *p_point_y, uint8_t point_num);
err_t B_GRAPH_DrawQbezier(uint8_t layer, st_graph_qbezier_info_t *qbezier_info);
err_t B_GRAPH_DrawArc(uint8_t layer, st_graph_arc_info_t *arc_info);
err_t B_GRAPH_CompositeBackData(st_image_object_t *p_input_img_obj, st_image_object_t *p_output_img_obj);



#ifdef __cplusplus
}
#endif

#endif // __B_GRAPHIC_API_H__

/*-----------------------------------------------------------------------------------------------*/
//	file end of "b_graphic_api.h"
/*-----------------------------------------------------------------------------------------------*/
