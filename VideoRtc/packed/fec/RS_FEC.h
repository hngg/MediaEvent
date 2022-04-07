
#ifndef __RS_FEC_H__
#define __RS_FEC_H__

#include "types.h"

#ifndef PJMEDIA_RTP_PT_HYT_FEC
#define PJMEDIA_RTP_PT_HYT_FEC 101
#endif

/*
*	Generating the logarithm tables of GF(2^4), GF(2^8) and GF(2^16).
*
*	w: 		The size of each words in bit
*	gflog: 	logarithm table
*	gfilog: logrithm inverse table
*/
int setup_logarithm_tables(int w, unsigned short *gflog, unsigned short *gfilog);

int gfadd(int a, int b);

int gfsub(int a, int b);

int gfmult(int a, int b, int w, unsigned short *gflog, unsigned short *gfilog);

int gfdiv(int a, int b, int w, unsigned short *gflog, unsigned short *gfilog);

int matrix_column_swap(int row_num, int column_num, int column1, int column2, unsigned short **matrix_A);

int matrix_column_gfmult(int row_num, int column_num, int column, int gain, unsigned short **matrix_A,
								  int w, unsigned short *gflog, unsigned short *gfilog);

int matrix_column_gfadd(int row_num, int column_num, int column_dst, int column_src, int src_gain,
								 unsigned short **matrix_A, int w, unsigned short *gflog, unsigned short *gfilog);

int Gaussian_elimination_by_column(int row_num, int column_num, unsigned short **matrix_A, int w, unsigned short *gflog, unsigned short *gfilog);

int gen_trans_matrix(int row_num, int column_num, unsigned short **matrix_A, int w, unsigned short *gflog, unsigned short *gfilog);

int get_pkt_seq(void* pkt);

pj_uint32_t get_pkt_ts(void* pkt);

int get_fec_pkt_SN(void* pkt, int fec_ext_len);

pj_uint16_t FEC_encode(pjsua_RS_FEC_config* fec_config, pj_uint16_t * packet, int packet_index, int pack_len, int fec_ext_len);

pj_uint16_t FEC_decode(pjsua_RS_FEC_config* fec_config, int index, int ts_span, int fec_ext_len);


#endif



