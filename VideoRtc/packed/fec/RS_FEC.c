
#include "glog.h"
#include "RS_FEC.h"
#include "rtp.h"
#include <netinet/in.h>

#ifndef RS_FEC_BIT_WIDTH_4
#define RS_FEC_BIT_WIDTH_4 4
#endif

#ifndef RS_FEC_BIT_WIDTH_8
#define RS_FEC_BIT_WIDTH_8 8
#endif

#ifndef RS_FEC_BIT_WIDTH_16
#define RS_FEC_BIT_WIDTH_16 16
#endif

int setup_logarithm_tables(int w, unsigned short *gflog, unsigned short *gfilog){
	unsigned int prim_poly_4 = 0x13;  		//corresponding to x^4 + x + 1
	unsigned int prim_poly_8 = 0x11D;		//corresponding to x^8 + x^4 + x^3 + x^2 + 1
	unsigned int prim_poly_16 = 0x1100B;	//corresponding to x^16 + x^12 + x^3 + x + 1

	unsigned int b, log, x_to_w, prim_poly;

	switch(w){
		case RS_FEC_BIT_WIDTH_4: prim_poly = prim_poly_4; break;
		case RS_FEC_BIT_WIDTH_8: prim_poly = prim_poly_8; break;
		case RS_FEC_BIT_WIDTH_16: prim_poly = prim_poly_16; break;
		default: return -1;
	}

	x_to_w = 1 << w;
	b = 1;
	for(log = 0; log < x_to_w - 1; log++){
		gflog[b] = (unsigned short) log;
		gfilog[log] = (unsigned short) b;
		b = b << 1;

		if(b & x_to_w)	b ^= prim_poly;
	}

	return 0;
}

int gfadd(int a, int b){
	return a^b;
}

int gfsub(int a, int b){
	return a^b;
}

int gfmult(int a, int b, int w, unsigned short *gflog, unsigned short *gfilog){
	int sum_log;
	int NW = 1 << w;

	if(w != RS_FEC_BIT_WIDTH_8 && w != RS_FEC_BIT_WIDTH_16){
		log_error("RS_FEC, Error gfmult width_in_bit:%d", w);
		return 0;
	}
	
	if(a < 0 || b < 0 || a > NW - 1 || b > NW - 1){
		log_error("RS_FEC, Error gfmult out of boundary, a:%d, b:%d", a, b);
		return 0;
	}
		
	if(0==a || 0 == b) return 0;
	sum_log = gflog[a]+gflog[b];
	if(sum_log >= NW - 1) sum_log -= NW - 1;
	return gfilog[sum_log];
}

int gfdiv(int a, int b, int w, unsigned short *gflog, unsigned short *gfilog){
	int diff_log;
	int NW = 1 << w;

	if(w != RS_FEC_BIT_WIDTH_8 && w != RS_FEC_BIT_WIDTH_16){
		log_error("RS_FEC, Error gfdiv width_in_bit:%d", w);
		return 0;
	}
	
	if(a < 0 || b < 0 || a > NW - 1 || b > NW - 1){
		log_error("RS_FEC, Error gfdiv out of boundary, a:%d, b:%d", a, b);
		return 0;
	}
	
	if(0==a) return 0;
	if(0==b){
		log_error("RS_FEC, Error gfdiv can't divide by 0");
		return 0;
	}
	diff_log = gflog[a]-gflog[b];
	if(diff_log < 0) diff_log += NW - 1;
	return gfilog[diff_log];
}

int matrix_column_swap(int row_num, int column_num, int column1, int column2, unsigned short **matrix_A){
	if(column1 < 0 || column1 >= column_num || column2 < 0 || column2 >= column_num || column1 == column2) return -1;

	int i, tmp;
	//the first column1-1 row is an identity matrix
	for(i = column1; i < row_num; i++){
		tmp = matrix_A[i][column1];
		matrix_A[i][column1] = matrix_A[i][column2];
		matrix_A[i][column2] = tmp;
	}
	return 0;
}

int matrix_column_gfmult(int row_num, int column_num, int column, int gain, unsigned short **matrix_A,
								  int w, unsigned short *gflog, unsigned short *gfilog){
	if(column < 0 || column >= column_num || gain == 0) return -1;
	
	int i;
	for(i = column; i < row_num; i++){
		matrix_A[i][column] = gfmult(matrix_A[i][column], gain, w, gflog, gfilog);
	}

	return 0;
}

int matrix_column_gfadd(int row_num, int column_num, int column_dst, int column_src, int src_gain,
								 unsigned short **matrix_A, int w, unsigned short *gflog, unsigned short *gfilog){
	if(column_dst < 0 || column_dst >= column_num || column_src < 0 || column_src >= column_num || src_gain == 0 || column_dst == column_src) return -1;

	int i;
	for(i = column_src; i < row_num; i++){
		matrix_A[i][column_dst] = gfadd(matrix_A[i][column_dst], gfmult(matrix_A[i][column_src], src_gain, w, gflog, gfilog));
	}

	return 0;
}

int Gaussian_elimination_by_column(int row_num, int column_num, unsigned short **matrix_A, int w, unsigned short *gflog, unsigned short *gfilog){
	int i,j,k;
	
	//transform the first column_num row to identity matrix
	for(i = 0; i < column_num; i++){
		if(0 == matrix_A[i][i]){
			for(k = i + 1; k < column_num; k++){
				if(0 != matrix_A[i][k]){
					matrix_column_swap(row_num, column_num, i, k, matrix_A);
					break;
				}
			}
			if(k == column_num) return -1;
		}

		if(1 != matrix_A[i][i]){
			matrix_column_gfmult(row_num, column_num, i, gfdiv(1, matrix_A[i][i], w, gflog, gfilog), matrix_A, w, gflog, gfilog);
		}

		for(j = 0; j < column_num; j++){
			if(i == j) continue;

			if(0 != matrix_A[i][j]){
				matrix_column_gfadd(row_num, column_num, j, i, matrix_A[i][j], matrix_A, w, gflog, gfilog);
			}
			
		}
	}
	return 0;
}
	

int gen_trans_matrix(int row_num, int column_num, unsigned short **matrix_A, int w, unsigned short *gflog, unsigned short *gfilog){
	int i, j;

	//generating row_num*column_num Vandermonda Matrix
	for(i = 0; i < row_num; i++){
		for(j = 0; j < column_num; j++){
			//the first column
			if(0 == j){
				matrix_A[i][j] = 1;
				continue;
			}

			matrix_A[i][j] = gfmult(matrix_A[i][j-1], i, w, gflog, gfilog);
		}
	}

	Gaussian_elimination_by_column(row_num, column_num, matrix_A, w, gflog, gfilog);

	return 0;	
}

int inverse_Matrix(int dimension, unsigned short **matrix_A, int w, unsigned short *gflog, unsigned short *gfilog){
	int i;

	//set the last column_num row to identity Matrix
	for(i=0; i<dimension; i++){
		bzero(matrix_A[dimension+i], dimension<<1);
		matrix_A[dimension+i][i] = 1;
	}

	Gaussian_elimination_by_column(dimension<<1, dimension, matrix_A, w, gflog, gfilog);

	return 0;
}

int get_pkt_seq(void* pkt){
	return ntohs( ((pjmedia_rtp_hdr*) pkt)->seq ); 
}

pj_uint32_t get_pkt_ts(void* pkt){
	return ntohl( ((pjmedia_rtp_hdr*) pkt)->ts ); 
}

int get_fec_pkt_SN(void* pkt, int fec_ext_len){
	pj_uint8_t *tmp_fec = (pj_uint8_t *)pkt;
	tmp_fec += sizeof(pjmedia_rtp_hdr) + fec_ext_len;
	return ntohs(((pjmedia_fec_hdr*)tmp_fec)->SN);
}

pj_uint16_t FEC_buf_init(pjsua_RS_FEC_config* fec_config){
	int i;
	
	for(i = 0; i < fec_config->fec_num; i++){
		bzero(fec_config->fec_buf[i], fec_config->fec_buf_size);
	}
	return 0;
}

pj_uint16_t FEC_encode(pjsua_RS_FEC_config* fec_config, pj_uint16_t * packet, int packet_index, int pack_len, int fec_ext_len){
	int i, j;
	
	if(packet_index == 0) FEC_buf_init(fec_config);

	if(pack_len > fec_config->fec_buf_size - sizeof(pjmedia_fec_hdr) - fec_ext_len){
		log_error("RS_FEC encode pack_len Error, pack_len:%d fec_buf_size:%d", pack_len, fec_config->fec_buf_size);
		return PJ_FALSE;
	}
		
	//PJ_LOG2(4,("RS_FEC","encode packet[%d], seq:%d, pack_len:%d", packet_index, get_pkt_seq(packet), pack_len));
	//print_value_every_8bit(packet, pack_len);
	
	//for each fec packet
	for(i = 0; i < fec_config->fec_num; i++){
		pj_uint16_t* enc_info = fec_config->matrix_A[fec_config->data_num+i];
		
		pj_uint8_t *offset = (pj_uint8_t *)fec_config->fec_buf[i];
		offset += sizeof(pjmedia_rtp_hdr) + fec_ext_len;
		pjmedia_fec_hdr *fec_hdr = (pjmedia_fec_hdr *)offset;

		pj_uint8_t *pack_offset = (pj_uint8_t *)packet;
		pjmedia_rtp_hdr *pack_hdr =  (pjmedia_rtp_hdr *)pack_offset;
		
		//encode fec_hdr
		fec_hdr->len_fec ^= gfmult(enc_info[packet_index], pack_len, fec_config->width_in_bit, fec_config->gflog, fec_config->gfilog);
		fec_hdr->pt_fec ^= gfmult(enc_info[packet_index], pack_hdr->pt, fec_config->width_in_bit, fec_config->gflog, fec_config->gfilog);

		if(packet_index == 0){
			fec_hdr->SN = pack_hdr->seq;
			fec_hdr->n = fec_config->data_num;
			fec_hdr->m = fec_config->fec_num;
		}
		//encode data
		pack_offset += sizeof(pjmedia_rtp_hdr);
		offset += sizeof(pjmedia_fec_hdr);
		
		//each word is 8bits
		if(RS_FEC_BIT_WIDTH_8 == fec_config->width_in_bit){
			for(j = 0; j < pack_len-sizeof(pjmedia_rtp_hdr); j++){				
				*offset ^= gfmult(enc_info[packet_index], *pack_offset, fec_config->width_in_bit, fec_config->gflog, fec_config->gfilog);
				offset ++;
				pack_offset ++;
			}
		}
		//each word is 16bits
		else if(RS_FEC_BIT_WIDTH_16 == fec_config->width_in_bit){
			pj_uint16_t* offset_16 = (pj_uint16_t*)offset;
			pj_uint16_t* pack_offset_16 = (pj_uint16_t*)pack_offset;
			
			for(j=0; j < (pack_len-sizeof(pjmedia_rtp_hdr))/2; j++){				
				*offset_16 ^= gfmult(enc_info[packet_index], *pack_offset_16, fec_config->width_in_bit, fec_config->gflog, fec_config->gfilog);
				offset_16 ++;
				pack_offset_16 ++;
			}

			//if pack_len is odd, set the last bytes zero.
			if(pack_len%2){
				//must notice the host order
				pj_uint16_t last_word;
				last_word = *pack_offset_16 & 0x00FF;

				*offset_16 ^= gfmult(enc_info[packet_index], last_word, fec_config->width_in_bit, fec_config->gflog, fec_config->gfilog);
			}
		}

		//set the padding of packets to zero if they are too short, data = data ^ gfmult(A[][], 0) = data ^ 0 = data, nothing should be done.
	}
	return PJ_TRUE;
}

pj_uint16_t FEC_decode(pjsua_RS_FEC_config* fec_cfg, int index, int ts_span, int fec_ext_len){
	int i, j, data_len;
	pj_uint16_t recover_pt = 0;
	pj_uint16_t recover_len = 0;

	pj_uint16_t* dec_info = fec_cfg->inverse_matrix_A[fec_cfg->data_num + fec_cfg->pack_lost_index[index]];
	
	for(i=0; i<fec_cfg->data_num; i++){
		pj_uint8_t *offset = fec_cfg->received_pack[i];
		pjmedia_rtp_hdr *pack_hdr =  (pjmedia_rtp_hdr *)offset;

		if(pack_hdr->pt == PJMEDIA_RTP_PT_HYT_FEC){
			offset += sizeof(pjmedia_rtp_hdr) + fec_ext_len;
			pjmedia_fec_hdr *fec_hdr =  (pjmedia_fec_hdr *)offset;

			recover_pt ^= gfmult(dec_info[i], ntohs(fec_hdr->pt_fec), fec_cfg->width_in_bit, fec_cfg->gflog, fec_cfg->gfilog);
			recover_len ^= gfmult(dec_info[i], ntohs(fec_hdr->len_fec), fec_cfg->width_in_bit, fec_cfg->gflog, fec_cfg->gfilog);

			//data: from rtp_ext to the end, skip fec_hdr
			data_len = fec_cfg->received_len[i] - sizeof(pjmedia_rtp_hdr) - fec_ext_len - sizeof(pjmedia_fec_hdr);
			offset += sizeof(pjmedia_fec_hdr);
		}
		else{
			recover_pt ^= gfmult(dec_info[i], pack_hdr->pt, fec_cfg->width_in_bit, fec_cfg->gflog, fec_cfg->gfilog);
			recover_len ^= gfmult(dec_info[i], fec_cfg->received_len[i], fec_cfg->width_in_bit, fec_cfg->gflog, fec_cfg->gfilog);

			offset += sizeof(pjmedia_rtp_hdr);
			data_len = fec_cfg->received_len[i] - sizeof(pjmedia_rtp_hdr);
		}

		pj_uint8_t *data_offset = (pj_uint8_t *)fec_cfg->recovered_data; 
		data_offset += sizeof(pjmedia_rtp_hdr);

		if(data_len > fec_cfg->recovered_data_buf_len - sizeof(pjmedia_rtp_hdr)){
			log_error("RS_FEC_D recover data_len Error, data_len:%d recovered_data_buf_len:%d", data_len, fec_cfg->recovered_data_buf_len);
			return 0;
		}
			
		//each word is 8bits
		if(RS_FEC_BIT_WIDTH_8 == fec_cfg->width_in_bit){
			for(j = 0; j < data_len; j++){				
				*data_offset ^= gfmult(dec_info[i], *offset, fec_cfg->width_in_bit, fec_cfg->gflog, fec_cfg->gfilog);
				offset++;
				data_offset++;
			}
		}
		//each word is 16bits
		else if(RS_FEC_BIT_WIDTH_16 == fec_cfg->width_in_bit){
			pj_uint16_t* offset_16 = (pj_uint16_t*)offset;
			pj_uint16_t* data_offset_16 = (pj_uint16_t*)data_offset;

			for(j=0; j < data_len/2; j++){
				*data_offset_16 ^= gfmult(dec_info[i], *offset_16, fec_cfg->width_in_bit, fec_cfg->gflog, fec_cfg->gfilog);
				offset_16++;
				data_offset_16++;
			}

			if(data_len%2){
				pj_uint16_t last_word;
				last_word = *offset_16 & 0x00FF;
				
				*data_offset_16 ^= gfmult(dec_info[i], last_word, fec_cfg->width_in_bit, fec_cfg->gflog, fec_cfg->gfilog);
			}
		}
	
	}

	pjmedia_rtp_hdr *data_rtp_hdr = (pjmedia_rtp_hdr *)fec_cfg->recovered_data;
	data_rtp_hdr->pt = recover_pt;
	data_rtp_hdr->seq = htons( fec_cfg->dec_fec_SN + fec_cfg->pack_lost_index[index]);

	pj_int64_t ts_diff = (ntohs(data_rtp_hdr->seq)-fec_cfg->ts_seq_record)*ts_span;
	data_rtp_hdr->ts = htonl( (pj_int64_t)fec_cfg->ts_record + ts_diff < 0 ? 0 : fec_cfg->ts_record + ts_diff);
	log_debug("RS_FEC_D, FEC_Recover FEC_decode SN:%d, lost_index:%d, recovered_seq:%d, recover_ts:%u",
		fec_cfg->dec_fec_SN, fec_cfg->pack_lost_index[index], ntohs(data_rtp_hdr->seq), ntohl(data_rtp_hdr->ts));

	return recover_len;
}
