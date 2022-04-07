#ifndef __VID_CODEC_H__
#define __VID_CODEC_H__

#include "types.h"

typedef struct pjmedia_frame
{
    void		    *buf;	    /**< Pointer to buffer.		    */
    unsigned int	size;	    /**< Frame size in bytes.		    */

	int             orientation;

    int             width;
    int             height;
    unsigned int    enc_packed_pos;
    pj_bool_t       has_more;
    pj_uint32_t		 bit_info;  /**< Bit info of the frame, sample case: a frame may not exactly start and end at the octet boundary, 
					                so this field may be used for specifying start & end bit offset. */
} pjmedia_frame;

typedef int (*on_vframe_slice)(char* frameBuffer, int frameLen, pjmedia_frame *out_package);

int get_h264_package(char* frameBuffer, int frameLen, pjmedia_frame *out_package);
int get_h265_package(char* frameBuffer, int frameLen, pjmedia_frame *out_package);

#endif
