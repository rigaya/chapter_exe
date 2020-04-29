// IIR_3DNR�t�B���^  by H_Kasahara(aka.HK) ���q��
// �V�[���`�F���W���o�p�ɃA���S���Y�����C by Yobi


//---------------------------------------------------------------------
//		�������������p
//---------------------------------------------------------------------

#include "stdafx.h"
#include <Windows.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

#define MAX_LINEOBJ		20		// �Œ胉�C�����o�����ʎ��͂���̌����͈�

#define FRAME_PICTURE	1
#define FIELD_PICTURE	2
#define MAX_SEARCH_EXTENT 32	//�S�T���̍ő�T���͈́B+-���̒l�܂ŁB
#define RATE_SCENE_CHANGE 100	//�V�[���`�F���W�Ɣ��肷�銄��x1000�B�w��l/1000�ȏ�̕ω�������΃V�[���`�F���W�Ƃ���
#define RATE_SCENE_CHGLOW 50	//���o�s�����������ɃV�[���`�F���W�Ɣ��肷�銄��x1000�B
#define RATE_SCENE_CHGBLK 20	//�󔒑������ɃV�[���`�F���W�Ɣ��肷�銄����x1000�B
#define RATE_SCENE_CHGCBK  8	//�󔒑������ɒ��S�t�߂������v�����ăV�[���`�F���W�Ɣ��肷�銄����x1000�B
#define RATE_SCENE_JDGBLK 850	//�󔒑����Ɣ��f����摜���󔒂̊�����x1000�B
#define RATE_SCENE_STAY   100	//�V�[���`�F���W�ł͂Ȃ��Ɣ��肷��Œ芄����x1000�B
#define RATE_SCENE_STYLOW 70	//�Œ�ӏ������ȏ㎞�ɃV�[���`�F���W�ł͂Ȃ��Ɣ��肷��Œ芄����x1000�B
#define RATE_SCENE_STYBLK 20	//�󔒑������ɃV�[���`�F���W�ł͂Ȃ��Ɣ��肷��Œ芄����x1000�B
#define RATE_SCENE_STYCBK  8	//�󔒑������ɒ��S�t�߂������v�����ăV�[���`�F���W�ł͂Ȃ��Ɣ��肷��Œ芄����x1000�B
#define THRES_STILLDATA    8    //�x�^�h��摜�p�Ɍ덷�͈͂Ɣ��f������K���Ȓl

//---------------------------------------------------------------------
//		�֐���`
//---------------------------------------------------------------------
//void make_motion_lookup_table();
//BOOL mvec(unsigned char* current_pix,unsigned char* bef_pix,int* vx,int* vy,int lx,int ly,int threshold,int pict_struct,int SC_level);
int mvec(int *mvec1,int *mvec2,int *flag_sc,unsigned char* current_pix,unsigned char* bef_pix,int lx,int ly,int threshold,int pict_struct, int nframe);
int search_change(int* val, unsigned char* pc, unsigned char* pb, int lx, int ly, int x, int y, int thres_fine, int thres_sc, int pict_struct);
int tree_search(unsigned char* current_pix,unsigned char* bef_pix,int lx,int ly,int *vx,int *vy,int search_block_x,int search_block_y,int min,int pict_struct, int method);
int full_search(unsigned char* current_pix,unsigned char* bef_pix,int lx,int ly,int *vx,int *vy,int search_block_x,int search_block_y,int min,int pict_struct, int search_extent);
int dist( unsigned char *p1, unsigned char *p2, int lx, int distlim, int block_height );
int maxmin_block( unsigned char *p, int lx, int block_height );
int avgdist( int *avg, unsigned char *psrc, int lx, int block_height );

//---------------------------------------------------------------------
//		�O���[�o���ϐ�
//---------------------------------------------------------------------
int	block_height, lx2;


//---------------------------------------------------------------------
//		�����덷����֐�
//---------------------------------------------------------------------
//[ru] �����x�N�g���̍��v��Ԃ�
//�Ԃ�l�̓V�[���`�F���W���萔�l�ɕύX
int tree=0, full=0;
int mvec(
		  int *mvec1,					//�C���^�[���[�X�œ������������̓������ʂ��i�[�i�o�́j
		  int *mvec2,					//�C���^�[���[�X�œ��������Ȃ����̓������ʂ��i�[�i�o�́j
          int *flag_sc,					//�V�[���`�F���W�t���O�i�o�́j
		  unsigned char* current_pix, 	//���t���[���̋P�x�B8�r�b�g�B
		  unsigned char* bef_pix,		//�O�t���[���̋P�x�B8�r�b�g�B
		  int lx,						//�摜�̉���
		  int ly,						//�摜�̏c��
		  int threshold,				//�������x�B(100-fp->track[1])*50 �c�c 50�͓K���Ȓl�B
		  int pict_struct,				//"1"�Ȃ�t���[�������A"2"�Ȃ�t�B�[���h����
		  int nframe )					// �t���[���ԍ��B�f�o�b�O�݂̂Ɏg�p
{
	int x, y;
	unsigned char *p1, *p2;
	int calc_total_lane_i0, calc_total_lane_i1;
	int rate_sc, rate_sc_all;
	int b_sc, b_sc_all;
	int thr_blank, thr_noobj, thr_mergin;

//�֐����Ăяo�����Ɍv�Z�����ɂ��ނ悤�O���[�o���ϐ��Ƃ���
	lx2 = lx*pict_struct;
	block_height = 16/pict_struct;

	// �V�[���`�F���W���o�p��臒l
	thr_blank  = threshold / 100;			// �󔒂Ɣ��肷��臒l
	thr_noobj  = threshold / 10;			// �\�����Ȃ��Ɣ��肷��臒l
	thr_mergin = threshold / 8;				// �O��t���[���덷�Ɣ��肷��臒l

	for(int i=0;i<pict_struct;i++)
	{
		int calc_total = 0;					// �����x�N�g���̍��v
		int areacnt_blankor = 0;			// �O��ǂ��炩�̃t���[�����󔒂̍��v
		int areacnt_blankand = 0;			// ���t���[������P�x�ŋ󔒂̍��v
		int areacnt_blank1 = 0;				// ���t���[�����󔒂̍��v
		int areacnt_blank2 = 0;				// �O�t���[�����󔒂̍��v
		int areacnt_noobj1 = 0;				// ���t���[���̕\�������Ȃ��n�_���v
		int areacnt_noobj2 = 0;				// �O�t���[���̕\�������Ȃ��n�_���v
		int cnt_total  = 0;					// �v�Z�̈搔
		int cnt_sc = 0;						// �V�[���`�F���W���o��
		int cnt_detobj = 0;					// �Œ�n�_���o��
		int cnt_center_detobj = 0;			// �Œ�n�_���o���i���S�t�߂̗̈�j
		int cnt_blank = 0;					// �󔒒n�_���o��
		int cnt_undet = 0;					// ���o��Ԃ������ŕs���Ȓn�_���o��
		int cnt_undetobj = 0;				// ���o��Ԃ������ŕs���Ȓn�_���o���i�\�����͖��m�ɑ��݁j
		int cnt_sc_low1 = 0;				// �\�������łɂ��V�[���`�F���W�n�_���o��
		int cnt_sc_low2 = 0;				// �\�����o���ɂ��V�[���`�F���W�n�_���o��
		int cnt_sc_center_low1 = 0;			// �\�������łɂ��V�[���`�F���W�n�_���o���i���S�t�߂̗̈�j
		int cnt_sc_center_low2 = 0;			// �\�����o���ɂ��V�[���`�F���W�n�_���o���i���S�t�߂̗̈�j
		// �Œ胉�C���v�Z������
		int lineobj[4][MAX_LINEOBJ];
		memset(lineobj, '\0', sizeof(int) * 4 * MAX_LINEOBJ);

		for(y=i+16;y<ly-16;y+=16)	//�S�̏c��
		{
			p1 = current_pix + y*lx + 16;
			p2 = bef_pix + y*lx + 16;
			for(x=16;x<lx-16;x+=16)	//�S�̉���
			{
				int center_area = 0;				// ���S�̈�ł̃J�E���g�p
				int invalid_th_fine = 0;			// ��v�������~�t���O
				int lowtype = 0;					// �\�����̏���(1)�A�o��(2)
				int ddist, ddist1, ddist2;			// ���S�ʒu�̋P�x�����
				int avg1, avg2;						// ���ϒl
				int th_fine;						// ��v��������臒l
				int nrank_sc;						// �V�[���`�F���W���o����
				int val_calc;						// �����x�N�g���ʕێ�
				unsigned char *pc, *pp;				// �摜�擪�ʒu

				// ���S�̈挟�o
				if (y >= ly*3/16 && y < ly - (ly*3/16) &&
					x >= lx*3/16 && x < lx - (lx*3/16)){
					center_area = 1;
				}

				// �P�t���[�����̍�����Βl���v�擾
				ddist1 = avgdist(&avg1, p1, lx2, block_height);		// ���t���[���̕��ς���̍�����Βl���v
				ddist2 = avgdist(&avg2, p2, lx2, block_height);		// �O�t���[���̕��ς���̍�����Βl���v
				// �O��t���[���̏�Ԃ𕪗�
				if (ddist1 <= threshold && ddist2 > thr_blank && ddist1 * 2 <= ddist2){
					lowtype = 1;								// ���t���[�����󔒂ɋ߂�
				}
				else if (ddist2 <= threshold && ddist1 > thr_blank && ddist2 * 2 <= ddist1){
					lowtype = 2;								// �O�t���[�����󔒂ɋ߂�
				}
				// �O��t���[���̋󔒁E�\�����Ȃ���Ԃ��J�E���g
				if (ddist1 <= thr_blank || ddist2 <= thr_blank){
					areacnt_blankor ++;							// �ǂ��炩�̃t���[������
					if (ddist1 <= thr_blank){
						areacnt_blank1 ++;						// ���t���[������
					}
					if (ddist2 <= thr_blank){
						areacnt_blank2 ++;						// �O�t���[������
					}
					if (ddist1 <= thr_blank && ddist2 <= thr_blank &&
						abs(avg1 - avg2) <= 5){
						areacnt_blankand ++;					// ���t���[���󔒂œ���P�x
					}
				}
				if (ddist1 <= thr_noobj){
					areacnt_noobj1 ++;							// ���t���[���̕\�������Ȃ�
				}
				if (ddist2 <= thr_noobj){
					areacnt_noobj2 ++;							// �O�t���[���̕\�������Ȃ�
				}

				// �O�t���[���\�������������ꍇ�����o���邽��
				// ���t���[���������󔒂ɋ߂��ꍇ�͑O�t���[���̓��������o
				if (lowtype == 1){					// �O�t���[����i�t�ݒ�j
					ddist = ddist2;
					pc    = p2;
					pp    = p1;
				}
				else{								// ���t���[����i�ʏ�ݒ�j
					ddist = ddist1;
					pc    = p1;
					pp    = p2;
				}
				// ��v��������臒l��ݒ�
				th_fine = ddist * 3 / 5;				// ��v�Ƃ���臒l�ݒ�
				if (th_fine > threshold){				// �ő�ŃV�[���`�F���W����
					th_fine = threshold;
				}
				if (th_fine <= thr_mergin){				// �덷�}�[�W���ȉ��̏ꍇ
					th_fine = threshold;				// ��v�����𒆎~����
					invalid_th_fine = 1;
				}

				// �V�[���`�F���W���o
				nrank_sc = search_change(&val_calc, pc, pp, lx, ly, x, y, th_fine, threshold, pict_struct);

				// �V�[���`�F���W���ʂ𕪗ނ��A���ތ��ʂ�+1�J�E���g
				// cnt_sc�͌��o�ɕK�{�A����ȊO�͔������p
				if (nrank_sc == 2){						// �V�[���`�F���W
					cnt_sc ++;
					// �󔒕������������̃V�[���`�F���W���o�p
					if (lowtype == 1){
						cnt_sc_low1 ++;
						if (center_area == 1) cnt_sc_center_low1 ++;
					}
					else if (lowtype == 2){
						cnt_sc_low2 ++;
						if (center_area == 1) cnt_sc_center_low2 ++;
					}
				}
				else if (ddist1 <= thr_blank && ddist2 <= thr_blank){	// ���t���[����
					cnt_blank ++;
				}
				else if (invalid_th_fine > 0 || nrank_sc > 0){		// �O��t���[����v��ԕs��
					cnt_undet ++;
					if (ddist1 > thr_noobj || ddist2 > thr_noobj){
						cnt_undetobj ++;			// ��v��ԕs���i�\�����͖��m�ɑ��݁j
					}
				}
				else{								// �O��t���[����v
					cnt_detobj ++;
					if (center_area == 1) cnt_center_detobj ++;
					// �Œ胉�C�����f�p
					if (center_area == 0){
						int idtmp;
						if (y < ly*3/16){			// ��ʏ㑤
							idtmp = y / 16;
							if (idtmp < MAX_LINEOBJ){
								lineobj[0][idtmp] ++;
							}
						}
						if (y >= ly - (ly*3/16)){	// ��ʉ���
							idtmp = (ly - y - 1) / 16;
							if (idtmp < MAX_LINEOBJ){
								lineobj[1][idtmp] ++;
							}
						}
						if (x < lx*3/16){			// ��ʍ���
							idtmp = x / 16;
							if (idtmp < MAX_LINEOBJ){
								lineobj[2][idtmp] ++;
							}
						}
						if (x >= lx - (lx*3/16)){	// ��ʉE��
							idtmp = (lx - x - 1) / 16;
							if (idtmp < MAX_LINEOBJ){
								lineobj[3][idtmp] ++;
							}
						}
					}
				}

				calc_total += val_calc;

				p1+=16;
				p2+=16;
				cnt_total ++;
			}
		}
		// �V�[���`�F���W�̊������v�Z
		rate_sc = cnt_sc * 1000 / cnt_total;

		//----- ��������������J�n�i�Ȃ��Ă����삷��j -----
		// �Œ胉�C�����o
		int cnt_lineobj = 0;		// �Œ蕔���̒��ŌŒ胉�C���ƔF��������
		if (1){						// �Œ胉�C�����o�������s
			int linecount_y = 0;
			// �㉺���E�̍��v�S�����Ń��C���Œ茟�o���s��
			for(int k1=0; k1<4; k1++){
				int cnt_lineobj_max;
				int cnttmp;
				int cnttmp_max;
				if (k1 < 2){		// �������C���̌��o
					cnt_lineobj_max = lx / 16;
				}
				else{				// �������C���̌��o
					cnt_lineobj_max = ly / 16;
				}
				// ���C���̔����ȏ�Œ�ň�ԑ��������o���Œ胉�C���Ƃ��Đݒ�
				cnttmp_max = 0;
				for(int k2=0; k2 < MAX_LINEOBJ; k2++){
					cnttmp = lineobj[k1][k2];
					if (cnttmp > cnt_lineobj_max / 2 && cnttmp > cnttmp_max){
						cnttmp_max = cnttmp;
					}
//if (cnttmp > cnt_lineobj_max / 2){
//printf("(%d:%d,%d,%d)",nframe,cnttmp_max,k1,k2);
//}
				}
				if (cnttmp_max > 1){
					cnt_lineobj += cnttmp_max;
					if (k1 < 2){			// �������C���ǉ��J�E���g
						linecount_y ++;
					}
					else{					// �������C�����͐������C���d���\����������
						cnt_lineobj -= linecount_y;
					}
				}
			}
			// �Œ胉�C�������̃f�[�^���O��
			if (cnt_lineobj > 0){
				cnt_detobj -= cnt_lineobj;
				cnt_total  -= cnt_lineobj;
			}
		}
		// �V�[���`�F���W�ȉ��̊����ł��V�[���`�F���W�ɂ���ꍇ�̔��f
		if (rate_sc < RATE_SCENE_CHANGE){
			int calc_jdgblk = cnt_total * RATE_SCENE_JDGBLK / 1000;
			int cnt_stayobj = cnt_detobj + cnt_undetobj;
			int flag_blankchk = 0;

			//--- �󔒒n�_��������Ώ��Ȃ��ω��ł��V�[���`�F���W�ɂ��鏈�� ---
			// �P�[�X�P�F�\���p���ꏊ�Ɣ�r���ďo�����ŃV�[���`�F���W�����������ꍇ�`�F�b�N���s
			if ((areacnt_blankor >= calc_jdgblk) &&
				(cnt_sc_low1*3/2 >= cnt_stayobj || cnt_sc_low2*3/2 >= cnt_stayobj)){
				flag_blankchk = 1;
			}
			// �P�[�X�Q�F�Е��̃t���[�����󔒂ŁA�Е��̃t���[�����󔒈ȊO�������ꍇ�`�F�b�N���s
			if ((areacnt_blank1 >= calc_jdgblk || areacnt_blank2 >= calc_jdgblk) &&
				(abs(areacnt_blank1 - areacnt_blank2) >= cnt_total / 3)){
				flag_blankchk = 1;
			}
			// �P�[�X�R�F�Е��̃t���[���̕\�������S���Ȃ��A�Е��̃t���[���̕\�����������ꍇ�`�F�b�N���s
			if ((areacnt_noobj1 >= calc_jdgblk || areacnt_noobj2 >= calc_jdgblk) &&
				(abs(areacnt_noobj1 - areacnt_noobj2) >= cnt_total / 2)){
				flag_blankchk = 1;
			}
			// �P�[�X�𖞂����m�F����ꍇ
			if (flag_blankchk > 0){
				// �󔒂��������̃V�[���`�F���W臒l�ݒ�
				int calc_blank_thres = cnt_total * RATE_SCENE_CHGBLK / 1000;
				// �\�����ړ����ł͂Ȃ��A�o���܂��͏��Ŏ��̂�
				if ((cnt_sc_low1 - (cnt_sc_low2 * 4) >= calc_blank_thres) ||
					(cnt_sc_low2 - (cnt_sc_low1 * 4) >= calc_blank_thres)){
						rate_sc = RATE_SCENE_CHANGE;
				}

				// ���S�������������ꍇ�̐ݒ�i�S�̖̂�S���G���A�j
				int calc_blank_thres_c = cnt_total * RATE_SCENE_CHGCBK / 1000;
				// �\�����ړ����ł͂Ȃ��A�o���܂��͏��Ŏ��̂�
				if ((cnt_sc_center_low1 - (cnt_sc_center_low2 * 4) >= calc_blank_thres_c) ||
					(cnt_sc_center_low2 - (cnt_sc_center_low1 * 4) >= calc_blank_thres_c)){
						rate_sc = RATE_SCENE_CHANGE;
				}
			}

			//--- �V�[���`�F���W����Œ�ӏ��������Ĉ��ȏ�̊���������ꍇ�̏��� ---
			if (cnt_sc - cnt_detobj/2 >= cnt_total * RATE_SCENE_CHGLOW / 1000 &&
				cnt_detobj <= cnt_total * RATE_SCENE_CHGLOW / 1000 &&
				rate_sc < RATE_SCENE_CHANGE){
				// ���o�s���ӏ���������΃V�[���`�F���W���Â߂ɐݒ肷�鏈��
				if (cnt_undet > cnt_total / 2){			// ���o�s���ӏ��������ȏ�
					rate_sc = RATE_SCENE_CHANGE;
				}
				// �󔒂������������o�s���ӏ���������ΊÂ߂ɐݒ�
				else if (areacnt_blankor >= calc_jdgblk &&
						 (cnt_sc - cnt_detobj + cnt_undet >= calc_jdgblk)){
					rate_sc = RATE_SCENE_CHANGE;
				}
			}
		}
		// �Œ�ӏ��̊�����臒l�ȏ゠��Ύ����I�ɃV�[���`�F���W��������
		if (cnt_detobj >= cnt_total * RATE_SCENE_STAY / 1000){
			// ���S�����ł̌Œ芄�����ɒ[�ɏ�������΃L�����Z��
			// ���͂ɌŒ�g�͗l�������Ă��V�[���`�F���W�O�ɂ͂��Ȃ��΍�
			if (cnt_center_detobj < cnt_total * RATE_SCENE_STYCBK / 1000){
			}
			else{
				rate_sc = rate_sc / 10;
			}
		}
		// �V�[���`�F���W�����𒴂��Ă��Ă��Œ肪������Ή������鏈��
		else if (rate_sc >= RATE_SCENE_CHANGE && rate_sc <= RATE_SCENE_CHANGE * 3){
			// �O��t���[�����ɋ󔒁{�P�x�ω��Ȃ��̗̈悪�����ȏ゠�������̕␳
			int calc_blank_dif  = abs(areacnt_blank1 - areacnt_blank2);
			int calc_blank_base = areacnt_blankand + calc_blank_dif;
			int calc_blank_rev  = (calc_blank_base - cnt_total/2) * 2 * RATE_SCENE_STAY / 1000;
			// �󔒁{�P�x�ω��Ȃ��������ȏ�̏ꍇ�͌Œ萔�␳��臒l�ȏ゠��΃V�[���`�F���W����
			if (areacnt_blankand >= cnt_total / 2 &&
				cnt_detobj >= cnt_total * RATE_SCENE_STYBLK / 1000 &&
				cnt_sc < calc_blank_dif + (cnt_total - calc_blank_base)/2 &&
				((cnt_detobj + calc_blank_rev >= cnt_total * RATE_SCENE_STAY / 1000) ||
				 calc_blank_base > cnt_total * RATE_SCENE_JDGBLK / 1000)){
				rate_sc = RATE_SCENE_CHANGE - 1;
			}
			// �Œ�ӏ��̊�����������x�ȏ゠�鎞�̓V�[���`�F���W�����߂ɐݒ�
			else if (cnt_detobj >= cnt_total * RATE_SCENE_STYLOW / 1000){
				if (cnt_sc - cnt_detobj + cnt_total * RATE_SCENE_STYLOW / 1000
					 < cnt_total * RATE_SCENE_CHANGE / 1000){
					rate_sc = RATE_SCENE_CHANGE - 1;
				}
			}
		}
		// �ω���������΋����V�[���`�F���W�ɂ���
		if (rate_sc >= RATE_SCENE_CHANGE && rate_sc <= RATE_SCENE_CHANGE * 3){
			if (cnt_sc + cnt_undetobj - (cnt_detobj * 10) >= cnt_total/2){
				rate_sc = RATE_SCENE_CHANGE * 3;
			}
		}
		//----- �����܂Ŕ������I�� -----

		// �V�[���`�F���W����
		if (rate_sc >= RATE_SCENE_CHANGE){
			b_sc = 1;
		}
		else{
			b_sc = 0;
		}
		if (0){	// debug
			if (nframe >= 6548 && nframe <= 6553 || nframe >= 19020 && nframe <= 19026 ||
				nframe >= 36396 && nframe <= 36396 || nframe >= 47105 && nframe <= 47123){
				printf("f:%d | %d %d,(%d,%d,[%d,%d,%d]),%d,%d,%d,%d,(%d,%d,%d,%d),%d,%d(%d)\n",
				rate_sc, nframe, cnt_sc, cnt_sc_low1, cnt_sc_low2,
				cnt_sc_center_low1, cnt_sc_center_low2, cnt_center_detobj,
				areacnt_blank1, areacnt_blank2, areacnt_blankand, areacnt_blankor,
				cnt_blank, cnt_detobj, cnt_undet, cnt_undetobj, cnt_lineobj, cnt_total,calc_total);}
		}
		// �C���^�[���[�X�̓g�b�v�^�{�g���ŕʁX�ɋL���B���v�Z�Ƌ�ʂōŒ�P�ȏ�ɂ��邽�߂P�����Z
		if (i == 0){
			calc_total_lane_i0 = calc_total+1;
			b_sc_all = b_sc;
			rate_sc_all = rate_sc;
		}
		else{
			calc_total_lane_i1 = calc_total+1;
			if (rate_sc_all < rate_sc){
				b_sc_all = b_sc;
				rate_sc_all = rate_sc;
			}
		}
	}

	if (pict_struct != 2){				// �t���[�������̏ꍇ�A���ʂ����̂܂ܑ��
		*mvec1 = calc_total_lane_i0;
		*mvec2 = calc_total_lane_i0;
		*flag_sc = b_sc_all;
	}
	else if (calc_total_lane_i0 >= calc_total_lane_i1){	// �C���^�[���[�X��lane0�̕��������傫����
		*mvec1 = calc_total_lane_i0;
		*mvec2 = calc_total_lane_i1;
		*flag_sc = b_sc_all;
	}
	else{												// �C���^�[���[�X��lane1�̕��������傫����
		*mvec1 = calc_total_lane_i1;
		*mvec2 = calc_total_lane_i0;
		*flag_sc = b_sc_all;
	}

	/*char str[500];
	sprintf_s(str, 500, "tree:%d, full:%d", tree, full);
	MessageBox(NULL, str, 0, 0);*/

	return rate_sc_all;
}

//---------------------------------------------------------------------
// �V�[���`�F���W�����o
// �o��
//   �Ԃ�l�F0=�O��t���[����v  1=�O��t���[����v�s��  2=�V�[���`�F���W
//   val   �F�����x�N�g����
//---------------------------------------------------------------------
int search_change(
	int* val,				//�����x�N�g���ʁi���ʂ̒l�j
	unsigned char* pc,		//���o���t���[���̋P�x�B8�r�b�g�B
	unsigned char* pp,		//���o��t���[���̋P�x�B8�r�b�g�B
	int lx,					//�摜�̉���
	int ly,					//�摜�̏c��
	int x,					//�����ʒu
	int y,					//�����ʒu
	int thres_fine,			//��r臒l�i��v����j
	int thres_sc,			//��r臒l�i�V�[���`�F���W����j
	int pict_struct)		//"1"�Ȃ�t���[�������A"2"�Ȃ�t�B�[���h����
{
	int method = 0;			//�����̊ȈՉ��i0:�T������ 1:�Q���T�� 2:�����ȗ��j
	int n_sc = 0;
	int vx = 0;
	int vy = 0;

	//���ʒu�ł̃t���[���Ԃ̐�Βl���B
	int min = dist( pc, pp, lx2, INT_MAX, block_height );
	if (min <= thres_fine){		//�t���[���Ԃ̐�Βl�����ŏ����珬������Ίȗ���
		//method = 1;		//���������l���ɓ����Ȃ炱����
		method = 2;			//���x�D��Ȃ炱����
	}
	// tree_search�͖{����v����臒l�܂ł����������A���x����̂��߃V�[���`�F���W臒l�܂łɂ���
	if( thres_sc < (min = tree_search( pc, pp, lx, ly, &vx, &vy, x, y, min, pict_struct, method))){
		//�t���[���Ԃ̐�Βl�����傫����ΑS�T���������Ȃ�
		if ( thres_sc < (min = full_search( pc, &pp[vy * lx + vx], lx, ly, &vx, &vy, x+vx, y+vy, min, pict_struct, max(abs(vx),abs(vy))*2 ))){
			// �ŏ��̌����͈͂ɂ�����Ȃ��������̂��߁A���ꂽ�͈͂�T��
			int vxe = 0;
			int vye = 0;
			if (thres_sc < (min = tree_search( pc, pp, lx, ly, &vxe, &vye, x, y, min, pict_struct, 3))){ 
				//�����x�N�g���̍��v���V�[���`�F���W���x���𒴂��Ă�����A�V�[���`�F���W�Ɣ��肵�đ傫�Ȓl��ݒ�
				vx = MAX_SEARCH_EXTENT * 10;		// �S�̂�臒l�ł����o�ł��Ȃ������ꍇ�A�傫�Ȓl��ݒ�
				vy = MAX_SEARCH_EXTENT * 10;
				n_sc = 2;
			}
		}
	}
	if (n_sc == 0 && min > thres_fine){		//�t���[���Ԃ̐�Βl�����傫����Έ�v�s���ɐݒ�
		n_sc = 1;
	}
	*val = abs(vx)+abs(vy);
	return n_sc;
}

//---------------------------------------------------------------------
//		�ȈՒT���@���������֐�
//      �����l�̏ꍇ�͒��S�ɋ߂�����I������
//---------------------------------------------------------------------
int tree_search(unsigned char* current_pix,	//���t���[���̋P�x�B8�r�b�g�B
				unsigned char* bef_pix,		//�O�t���[���̋P�x�B8�r�b�g�B
				int lx,						//�摜�̉���
				int ly,						//�摜�̏c��
				int *vx,					//x�����̓����x�N�g������������B
				int *vy,					//y�����̓����x�N�g������������B
				int search_block_x,			//�����ʒu
				int search_block_y,			//�����ʒu
				int min,					//���ʒu�ł̃t���[���Ԃ̐�Βl���B�֐����ł͓��ʒu�̔�r�����Ȃ��̂ŁA�Ăяo���O�ɍs���K�v����B
				int pict_struct,			//"1"�Ȃ�t���[�������A"2"�Ȃ�t�B�[���h����
				int method)					//�����̊ȈՉ��i0:�T������ 1:�Q���T�� 2:�����ȗ� 3:�T�����񐔊O���j
{
	tree++;
	int dx, dy, ddx=0, ddy=0, xs=0, ys;
	int d;
	int x,y;
	int locx, locy;
	int loopmax, inter;
	int nrep, step, dthres;
	int speedup = pict_struct-1;
//�����͈͂̏���Ɖ�����ݒ�
	int ylow  = 0 - search_block_y;
	int yhigh = ly- search_block_y-16;
	int xlow  = 0 - search_block_x;
	int xhigh = lx- search_block_x-16;

	if (method == 2) return min;	// �����ȗ�

	if (method == 0){
		loopmax = 3-speedup;
		inter = 0;					// inter�͕s�g�p
	}
	else if (method == 3){
		loopmax = 1;
		inter = 0;					// inter�͕s�g�p
	}
	else{
		loopmax = 5-speedup;		// MAX_SEARCH_EXTENT=32�̎��i�v�Z�ȗ��̂��ߒ��ڒ�`�j
		inter = MAX_SEARCH_EXTENT;
	}
	for(int i=0; i<loopmax; i++){
		if (method == 0){			// �Q�i�K�Ō����i�t�B�[���h�����Ŕ�r���v�X�U��j
			if (i==0){
				locx = MAX_SEARCH_EXTENT - 8;
				locy = MAX_SEARCH_EXTENT - 8;
				nrep = MAX_SEARCH_EXTENT/8*2 - 1;
				step = 8;
				dthres = THRES_STILLDATA << 4;		// �덷�͈͂Ƃ���K���Ȓl
			}
			else if (i==1){
				locx = ddx - 6;
				locy = ddy - 6;
				nrep = 7;
				step = 2;
				dthres = THRES_STILLDATA << 2;		// �덷�͈͂Ƃ���K���Ȓl
			}
			else{
				locx = ddx - 1;
				locy = ddy - 1;
				nrep = 3;
				step = 1;
				dthres = 1;			// �덷�͈͂Ƃ���K���Ȓl
			}
		}
		else if (method == 3){
			locx = ddx - MAX_SEARCH_EXTENT*2;
			locy = ddy - MAX_SEARCH_EXTENT*2;
			nrep = 5;
			step = MAX_SEARCH_EXTENT;
			dthres = THRES_STILLDATA << 4;		// �덷�͈͂Ƃ���K���Ȓl
		}
		else{						// �Q���T���i�t�B�[���h�����Ŕ�r���v�R�Q��j
			inter = inter / 2;
			locx = ddx - inter;
			locy = ddy - inter;
			nrep = 3;
			step = inter;
			dthres = THRES_STILLDATA << (loopmax - i - 1);		// �덷�͈͂Ƃ���K���Ȓl
		}
		// �����J�n
		dy = locy;
		for(y=0; y<nrep; y++){
			if ( dy<ylow || dy>yhigh ){			//�����ʒu����ʊO�ɏo�Ă����猟���������Ȃ�Ȃ��B
			}
			else{
				ys = dy * lx;	//�����ʒu�c��
				dx = locx;
				for(x=0; x<nrep; x++){
					if( dx<xlow || dx>xhigh ){	//�����ʒu����ʊO�ɏo�Ă����猟���������Ȃ�Ȃ��B
					}
					else if (x == (nrep-1)/2 && y == (nrep-1)/2){	// ���S���W�ł͌v�Z���Ȃ��B
					}
					else{
						d = dist( current_pix, &bef_pix[ys+dx], lx2, min, block_height );
						if( d <= min ){	//����܂ł̌������t���[���Ԃ̐�Βl���������������炻�ꂼ�����B
							if ((d + dthres <= min) ||
								(abs(dx) + abs(dy) <= abs(ddx) + abs(ddy))){	// ���S�ɋ߂����A�덷臒l�ȏ㍷������ꍇ�Z�b�g
									min = d;
									ddx = dx;
									ddy = dy;
							}
						}
					}
					dx += step;
				}
			}
			dy += step;
		}
	}

	if(pict_struct==FIELD_PICTURE){
		for(x=0,dx=ddx-1;x<3;x+=2,dx+=2){
			if( search_block_x+dx<0 || search_block_x+dx+16>lx )	continue;	//�����ʒu����ʊO�ɏo�Ă����猟���������Ȃ�Ȃ��B
			d = dist( current_pix, &bef_pix[ys+dx], lx2, min, block_height );
			if( d < min ){	//����܂ł̌������t���[���Ԃ̐�Βl���������������炻�ꂼ�����B
				min = d;
				ddx = dx;
			}
		}
	}
	

	*vx += ddx;
	*vy += ddy;

	return min;
}
//---------------------------------------------------------------------
//		�S�T���@���������֐�
//      �����l�̏ꍇ�͒��S�ɋ߂�����I������
//---------------------------------------------------------------------
int full_search(unsigned char* current_pix,	//���t���[���̋P�x�B8�r�b�g�B
				unsigned char* bef_pix,		//�O�t���[���̋P�x�B8�r�b�g�B
				int lx,						//�摜�̉���
				int ly,						//�摜�̏c��
				int *vx,					//x�����̓����x�N�g������������B
				int *vy,					//y�����̓����x�N�g������������B
				int search_block_x,			//�����ʒu
				int search_block_y,			//�����ʒu
				int min,					//�t���[���Ԃ̐�Βl���B�ŏ��̒T���ł�INT_MAX�������Ă���B
				int pict_struct,			//"1"�Ȃ�t���[�������A"2"�Ȃ�t�B�[���h����
				int search_extent)			//�T���͈́B
{
	full++;
	int dx, dy, ddx=0, ddy=0;
	int d;
	int dthres;
//	int search_point;
	unsigned char* p2;

	if(search_extent>MAX_SEARCH_EXTENT)
		search_extent = MAX_SEARCH_EXTENT;

//�����͈͂̏���Ɖ������摜����͂ݏo���Ă��Ȃ����`�F�b�N
	int ylow  = 0 - ( (search_block_y-search_extent<0) ? search_block_y : search_extent );
	int yhigh = (search_block_y+search_extent+16>ly) ? ly-search_block_y-16 : search_extent;
	int xlow  = 0 - ( (search_block_x-search_extent<0) ? search_block_x : search_extent );
	int xhigh = (search_block_x+search_extent+16>lx) ? lx-search_block_x-16 : search_extent;

	dthres = THRES_STILLDATA;		// �덷�͈͂Ƃ���K���Ȓl
	for(dy=ylow;dy<=yhigh;dy+=pict_struct)
	{
		p2 = bef_pix + dy*lx + xlow;	//Y�������ʒu�Bxlow�͕��̒l�Ȃ̂�"p2=bef_pix+dy*lx-xlow"�Ƃ͂Ȃ�Ȃ�
		for(dx=xlow;dx<=xhigh;dx++)
		{
			d = dist( current_pix, p2, lx2, min, block_height );
			if(d <= min)	//����܂ł̌������t���[���Ԃ̐�Βl���������������炻�ꂼ�����B
			{
				if ((d + dthres <= min) ||
					(abs(dx) + abs(dy) <= abs(ddx) - abs(ddy))){	// ���S�ɋ߂����A�덷臒l�ȏ㍷������ꍇ�Z�b�g
					min = d;
					ddx = dx;
					ddy = dy;
				}
			}
			p2++;
		}
	}

	*vx += ddx;
	*vy += ddy;

	return min;
}
//---------------------------------------------------------------------
//		�t���[���Ԑ�Βl�����v�֐�
//---------------------------------------------------------------------
//bbMPEG�̃\�[�X�𗬗p
#include <emmintrin.h>

int dist( unsigned char *p1, unsigned char *p2, int lx, int distlim, int block_height )
{
	if (block_height == 8) {
		__m128i a, b, r;

		a = _mm_load_si128 ((__m128i*)p1 +  0);
		b = _mm_loadu_si128((__m128i*)p2 +  0);
		r = _mm_sad_epu8(a, b);

		a = _mm_load_si128 ((__m128i*)(p1 + lx));
		b = _mm_loadu_si128((__m128i*)(p2 + lx));
		r = _mm_add_epi32(r, _mm_sad_epu8(a, b));

		a = _mm_load_si128 ((__m128i*)(p1 + 2*lx));
		b = _mm_loadu_si128((__m128i*)(p2 + 2*lx));
		r = _mm_add_epi32(r, _mm_sad_epu8(a, b));

		a = _mm_load_si128 ((__m128i*)(p1 + 3*lx));
		b = _mm_loadu_si128((__m128i*)(p2 + 3*lx));
		r = _mm_add_epi32(r, _mm_sad_epu8(a, b));

		a = _mm_load_si128 ((__m128i*)(p1 + 4*lx));
		b = _mm_loadu_si128((__m128i*)(p2 + 4*lx));
		r = _mm_add_epi32(r, _mm_sad_epu8(a, b));

		a = _mm_load_si128 ((__m128i*)(p1 + 5*lx));
		b = _mm_loadu_si128((__m128i*)(p2 + 5*lx));
		r = _mm_add_epi32(r, _mm_sad_epu8(a, b));

		a = _mm_load_si128 ((__m128i*)(p1 + 6*lx));
		b = _mm_loadu_si128((__m128i*)(p2 + 6*lx));
		r = _mm_add_epi32(r, _mm_sad_epu8(a, b));

		a = _mm_load_si128 ((__m128i*)(p1 + 7*lx));
		b = _mm_loadu_si128((__m128i*)(p2 + 7*lx));
		r = _mm_add_epi32(r, _mm_sad_epu8(a, b));
		return _mm_extract_epi16(r, 0) + _mm_extract_epi16(r, 4);;
	}

	int s = 0;
	for(int i=0;i<block_height;i++)
	{
		/*
		s += motion_lookup[p1[0]][p2[0]];
		s += motion_lookup[p1[1]][p2[1]];
		s += motion_lookup[p1[2]][p2[2]];
		s += motion_lookup[p1[3]][p2[3]];
		s += motion_lookup[p1[4]][p2[4]];
		s += motion_lookup[p1[5]][p2[5]];
		s += motion_lookup[p1[6]][p2[6]];
		s += motion_lookup[p1[7]][p2[7]];
		s += motion_lookup[p1[8]][p2[8]];
		s += motion_lookup[p1[9]][p2[9]];
		s += motion_lookup[p1[10]][p2[10]];
		s += motion_lookup[p1[11]][p2[11]];
		s += motion_lookup[p1[12]][p2[12]];
		s += motion_lookup[p1[13]][p2[13]];
		s += motion_lookup[p1[14]][p2[14]];
		s += motion_lookup[p1[15]][p2[15]];*/

		__m128i a = _mm_load_si128((__m128i*)p1);
		__m128i b = _mm_loadu_si128((__m128i*)p2);
		__m128i r = _mm_sad_epu8(a, b);
		s += _mm_extract_epi16(r, 0) + _mm_extract_epi16(r, 4);

		if (s > distlim)	break;

		p1 += lx;
		p2 += lx;
	}
	return s;
}


//---------------------------------------------------------------------
//		�t���[���Ԑ�Βl�����v�֐�(SSE�o�[�W����)
//---------------------------------------------------------------------
int dist_SSE( unsigned char *p1, unsigned char *p2, int lx, int distlim, int block_height )
{
	int s = 0;
/*
dist_normal������ƕ�����悤�ɁAp1��p2�̐�Βl���𑫂��Ă��Adistlim�𒴂����炻�̍��v��Ԃ������B
block_height�ɂ�8��16���������Ă���A�O�҂̓t�B�[���h�����A��҂��t���[�������p�B
block_height��8���������Ă�����΁Alx�ɂ͉摜�̉������������Ă���B
block_height��16���������Ă�����΁Alx�ɂ͉摜�̉����̓�{�̒l���������Ă���B
�ǂȂ����A�������쐬���Ă�����������΁A���Ɋ��ӂ������܂��B
*/
	return s;
}


//---------------------------------------------------------------------
//		�u���b�N���̍ő�P�x���擾�֐�
//---------------------------------------------------------------------
int maxmin_block( unsigned char *p, int lx, int block_height )
{
	__m128i rmin, rmax, a, b, z;

	// �e��̍ő�E�ŏ������߂�
	rmin = _mm_load_si128((__m128i*)p);
	rmax = _mm_load_si128((__m128i*)p);
	p += lx;
	for(int i=1; i<block_height; i++){
		a = _mm_load_si128((__m128i*)p);
		rmin = _mm_min_epu8(rmin, a);
		rmax = _mm_max_epu8(rmax, a);
		p += lx;
	}
	// ��Ԃ̍ő�E�ŏ������߂�
	// 16�f�[�^�̍ő�E�ŏ����W�f�[�^�ɍi��
	z    = _mm_setzero_si128();
	a    = _mm_unpackhi_epi8(rmin, z);
	b    = _mm_unpacklo_epi8(rmin, z);
	rmin = _mm_min_epi16(a, b);
	a    = _mm_unpackhi_epi8(rmax, z);
	b    = _mm_unpacklo_epi8(rmax, z);
	rmax = _mm_max_epi16(a, b);
	// 8����4
	a    = _mm_unpackhi_epi16(rmin, z);
	b    = _mm_unpacklo_epi16(rmin, z);
	rmin = _mm_min_epi16(a, b);
	a    = _mm_unpackhi_epi16(rmax, z);
	b    = _mm_unpacklo_epi16(rmax, z);
	rmax = _mm_max_epi16(a, b);
	// 4����2
	a    = _mm_unpackhi_epi32(rmin, z);
	b    = _mm_unpacklo_epi32(rmin, z);
	rmin = _mm_min_epi16(a, b);
	a    = _mm_unpackhi_epi32(rmax, z);
	b    = _mm_unpacklo_epi32(rmax, z);
	rmax = _mm_max_epi16(a, b);
	// 2����1
	a    = _mm_unpackhi_epi64(rmin, z);
	b    = _mm_unpacklo_epi64(rmin, z);
	rmin = _mm_min_epi16(a, b);
	a    = _mm_unpackhi_epi64(rmax, z);
	b    = _mm_unpacklo_epi64(rmax, z);
	rmax = _mm_max_epi16(a, b);
	// ���ʎ��o��
	int val_min = _mm_extract_epi16(rmin, 0);
	int val_max = _mm_extract_epi16(rmax, 0);

	return val_max - val_min;
}

//---------------------------------------------------------------------
//		�t���[�������ϒl����̐�Βl�����v�֐�
//---------------------------------------------------------------------
int avgdist( int *avg, unsigned char *psrc, int lx, int block_height )
{
	__m128i a, b, r;
	unsigned char *p;
	int sum;
	unsigned char d_avg;

	// ���[�v�Q��Ō��ʂ��擾
	// �P��ځF���ϒl���擾
	// �Q��ځF���ϒl����̐�Βl�����v���擾

	b = _mm_setzero_si128();				// ���ϒl�擾�p�̔�r�l
	for(int i=0; i<2; i++){
		p = psrc;							// �擾�t���[���J�n�ʒu
		r = _mm_setzero_si128();			// ���ʏ�����
		for(int j=0; j<block_height; j++){
			a = _mm_loadu_si128((__m128i*)p);
			r = _mm_add_epi32(r, _mm_sad_epu8(a, b));
			p += lx;
		}
		sum = _mm_extract_epi16(r, 0) + _mm_extract_epi16(r, 4);

		// �P��ڂ̌��ʂ͂Q��ڂ̔�r�Ώےl�Ƃ���i���ϒl�Z�o�{����j
		if (i == 0){
			d_avg = (unsigned char) ((sum + (block_height * 16/2)) / (block_height * 16));
			b = _mm_set1_epi8(d_avg);
		}
	}
	*avg = d_avg;
	return sum;
}

