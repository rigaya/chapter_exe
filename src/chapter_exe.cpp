// chapter_exe.cpp : �R���\�[�� �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
//

#include "stdafx.h"
#include "source.h"
#include "faw.h"

// mvec.c
#define FRAME_PICTURE	1
#define FIELD_PICTURE	2
int mvec(int *mvec1,int *mvec2,int *flag_sc,unsigned char* current_pix,unsigned char* bef_pix,int lx,int ly,int threshold,int pict_struct, int nframe);

// �P��̖������ԓ��ɕێ�����ő�V�[���`�F���W��
#define DEF_SCMAX 100

int proc_scene_change(
	Source *video,int *lastmute_scpos,int *lastmute_marker,FILE *fout,unsigned char *pix0,unsigned char *pix1,
	int w,int h,int start_fr,int seri,int setseri,int breakmute,int extendmute,int debug,int idx);


// �ʏ�̏o��
void write_chapter(FILE *f, int nchap, int frame, TCHAR *title, INPUT_INFO *iip) {
	LONGLONG t,h,m;
	double s;

	t = (LONGLONG)frame * 10000000 * iip->scale / iip->rate;
	h = t / 36000000000;
	m = (t - h * 36000000000) / 600000000;
	s = (t - h * 36000000000 - m * 600000000) / 10000000.0;

	fprintf(f, "CHAPTER%02d=%02d:%02d:%06.3f\n", nchap, (int)h, (int)m, s);
	fprintf(f, "CHAPTER%02dNAME=%s\n", nchap, title);
	fflush(f);
}
// ��͗p�̏o��
void write_chapter_debug(FILE *f, int nchap, int frame, TCHAR *title, INPUT_INFO *iip) {
	LONGLONG t,h,m;
	double s;

	t = (LONGLONG)frame * 10000000 * iip->scale / iip->rate;
	h = t / 36000000000;
	m = (t - h * 36000000000) / 600000000;
	s = (t - h * 36000000000 - m * 600000000) / 10000000.0;

	fprintf(f, "CHAPTER%02d=%02d:%02d:%06.3f from:%d\n", nchap, (int)h, (int)m, s, frame);   // add "from:%d" for debug
//	fprintf(f, "CHAPTER%02d=%02d:%02d:%06.3f\n", nchap, (int)h, (int)m, s);
	fprintf(f, "CHAPTER%02dNAME=%s\n", nchap, title);
	fflush(f);
}

int _tmain(int argc, _TCHAR* argv[])
{
	// ���������[�N�`�F�b�N
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	printf(_T("chapter.auf pre loading program.\n"));
	printf(_T("usage:\n"));
	printf(_T("\tchapter_exe.exe -v input_avs -o output_txt\n"));
	printf(_T("params:\n\t-v ���͉摜�t�@�C��\n\t-a ���͉����t�@�C���i�ȗ����͓���Ɠ����t�@�C���j\n\t-m ��������臒l�i1�`2^15)\n\t-s �Œᖳ���t���[����\n\t-b �����V�[�������Ԋu��\n"));
	printf(_T("\t-e �����O�㌟���g���t���[����\n"));

	TCHAR *avsv = NULL;
	TCHAR *avsa = NULL;
	TCHAR *out =  NULL;
	short setmute = 50;
	int setseri = 10;
	int breakmute = 60;
	int extendmute = 1;
	int thin_audio_read = 1;
	int debug = 0;

	for(int i=1; i<argc-1; i++) {
		char *s	= argv[i];
		if (s[0] == '-') {
			switch(s[1]) {
			case 'v':
				avsv = argv[i+1];
				if (strlen(s) > 2 && s[2] == 'a') {
					avsa = argv[i+1];
				}
				i++;
				break;
			case 'a':
				avsa = argv[i+1];
				i++;
				break;
			case 'o':
				out = argv[i+1];
				i++;
				break;
			case 'm':
				setmute = atoi(argv[i+1]);
				i++;
				break;
			case 's':
				setseri = atoi(argv[i+1]);
				i++;
				break;
			case 'b':
				breakmute = atoi(argv[i+1]);
				i++;
				break;
			case 'e':
				extendmute = atoi(argv[i+1]);
				i++;
				break;
			case '-':
				if (strcmp(&s[2], _T("debug")) == 0){
					debug = 1;
				}
				else if (strcmp(&s[2], _T("thin")) == 0){
					thin_audio_read = 2;
				}
				else if (strcmp(&s[2], _T("serial")) == 0){
					thin_audio_read = -1;
				}
				break;
			default:
				printf("error: unknown param: %s\n", s);
				break;
			}
		} else {
			printf("error: unknown param: %s\n", s);
		}
	}

	// �������͂������ꍇ�͓�����ɂ���Ɖ���
	if (avsa == NULL) {
		avsa = avsv;
	}

	if (out == NULL) {
		printf("error: no output file path!");
		return -1;
	}

	printf(_T("Setting\n"));
	printf(_T("\tvideo: %s\n\taudio: %s\n\tout: %s\n"), avsv, (strcmp(avsv, avsa) ? avsa : "(within video source)"), out);
	printf(_T("\tmute: %d seri: %d bmute: %d emute: %d\n"), setmute, setseri, breakmute, extendmute);

	printf("Loading plugins.\n");

	Source *video = NULL;
	Source *audio = NULL;
	try {
		AuiSource *srcv = new AuiSource();
		srcv->init(avsv);
		if (srcv->has_video() == false) {
			srcv->release();
			throw "Error: No Video Found!";
		}
		video = srcv;
		// �����\�[�X�̏ꍇ�͓����C���X�^���X�œǂݍ���
		if (strcmp(avsv, avsa) == 0 && srcv->has_audio()) {
			audio = srcv;
			audio->add_ref();
		}

		// �������ʃt�@�C���̎�
		if (audio == NULL) {
			if (strlen(avsa) > 4 && _stricmp(".wav", avsa + strlen(avsa) - 4) == 0) {
				// wav
				WavSource *wav = new WavSource();
				wav->init(avsa);
				if (wav->has_audio()) {
					audio = wav;
					audio->set_rate(video->get_input_info().rate, video->get_input_info().scale);
				} else {
					wav->release();
				}
			} else {
				// aui
				AuiSource *aud = new AuiSource();
				aud->init(avsa);
				if (aud->has_audio()) {
					audio = aud;
					audio->set_rate(video->get_input_info().rate, video->get_input_info().scale);
				} else {
					aud->release();
				}
			}
		}

		if (audio == NULL) {
			throw "Error: No Audio!";
		}
	} catch(char *s) {
		if (video) {
			video->release();
		}
		printf("%s\n", s);
		return -1;
	}

	// ������lwinput.aui�������ꍇ�͘A���ǂݏo�����������肷��̂ŊԈ����������ǂݍ���
	if (strstr(avsa, "lwinput.aui://") != NULL){
		if (thin_audio_read == 1){
			thin_audio_read = 0;
		}
	}

	FILE *fout;
	if (fopen_s(&fout, out, "w") != 0) {
		printf("Error: output file open failed.");
		video->release();
		audio->release();
		return -1;
	}

	INPUT_INFO &vii = video->get_input_info();
	INPUT_INFO &aii = audio->get_input_info();

	printf(_T("Movie data\n"));
	printf(_T("\tVideo Frames: %d [%.02ffps]\n"), vii.n, (double)vii.rate / vii.scale);
	DWORD fcc = vii.handler;
	printf(_T("\tVideo Format: %c%c%c%c\n"), fcc & 0xFF, fcc >> 8 & 0xFF, fcc >> 16 & 0xFF, fcc >> 24);

	printf(_T("\tAudio Samples: %d [%dHz]\n"), aii.audio_n, aii.audio_format->nSamplesPerSec);

	if (fcc == 0x32424752 || fcc == 0x38344359) {
		printf(_T("Error: Unsupported color RGB/YC48."));
	}

	if (fcc != 0x32595559) {
		printf(_T("warning: only YUY2 is supported. continues...\n"));
		//return -1;
	}

	short buf[4800*2]; // 10fps�ȏ�
	int n = vii.n;

	// FAW check
	do {
		CFAW cfaw;
		int faws = 0;

		for (int i=0; i<min(90, n); i++) {
			int naudio = audio->read_audio(i, buf);
			int j = cfaw.findFAW(buf, naudio);
			if (j != -1) {
				cfaw.decodeFAW(buf+j, naudio-j, buf); // test decode
				faws++;
			}
		}
		if (faws > 5) {
			if (cfaw.isLoadFailed()) {
				printf("  Error: FAW detected, but no FAWPreview.auf.\n");
			} else {
				printf("  FAW detected.\n");
				audio = new FAWDecoder(audio);
			}
		}
	} while(0);

	if (thin_audio_read <= 0){
		printf("read audio : serial\n");
	}
	printf(_T("--------\nStart searching...\n"));

	short mute = setmute;
	int seri = 0;
	int cnt_mute = 0;
	int idx = 1;
	int volume;
	int lastmute_scpos = -1;			// -e�I�v�V�����̌����I�[�o�[���b�v���l�����đO��ʒu�ێ�
	int lastmute_marker = -1;			// �}�[�N�\���p�̋N�_�ʒu�ێ�

	// start searching
	for (int i=0; i<n; i++) {
		// searching foward frame
		if (seri == 0 && thin_audio_read > 0) {		// �Ԉ������Ȃ��疳���m�F
			int naudio = audio->read_audio(i+setseri-1, buf);

			bool skip = false;
			for (int j=0; j<naudio; ++j) {
				volume = abs(buf[j]);
				//if (abs(buf[j]) > mute) {
				if (volume > mute) {
					skip = true;
					break;
				}
			}
			if (skip) {
				i += setseri;
			}
		}

		bool nomute = false;
		int naudio = audio->read_audio(i, buf);

		for (int j=0; j<naudio; ++j) {
			volume = abs(buf[j]);
			//if (abs(buf[j]) > mute) {
			if (volume > mute) {
				nomute = true;
				break;
			}
		}

		//
		if (nomute || i == n-1) {
			// owata
			if (seri >= setseri) {
				int start_fr = i - seri;

				printf(_T("mute%2d: %d - %d�t���[��\n"), idx, start_fr, seri);

				int w = vii.format->biWidth & 0xFFFFFFF0;
				int h = vii.format->biHeight & 0xFFFFFFF0;
				unsigned char *pix0 = (unsigned char*)_aligned_malloc(1920*1088, 32);
				unsigned char *pix1 = (unsigned char*)_aligned_malloc(1920*1088, 32);

				//--- ��ԓ��̃V�[���`�F���W���擾 ---
				proc_scene_change(video, &lastmute_scpos, &lastmute_marker, fout, pix0, pix1, w, h,
									start_fr, seri, setseri, breakmute, extendmute, debug, idx);


				idx++;

				_aligned_free(pix0);
				_aligned_free(pix1);
			}
			seri = 0;
		} else {
			seri++;
		}
	}

	// �ŏI�t���[���ԍ����o�́i�����łŒǉ��j
	fprintf(fout, "# SCPos:%d %d\n", n-1, n-1);

	// �\�[�X�����
	video->release();
	audio->release();

	return 0;
}


// ��ԓ��̃V�[���`�F���W���擾�E�o��
int proc_scene_change(
	Source *video,					// �摜�N���X
	int *lastmute_scpos,			// -e�I�v�V�����̌����I�[�o�[���b�v���l�����đO��ʒu�ێ��i�㏑���X�V�j
	int *lastmute_marker,			// �}�[�N�\���p�̋N�_�ʒu�ێ��i�㏑���X�V�j
	FILE *fout,						// �o�̓t�@�C��
	unsigned char *pix0,			// �摜�f�[�^�ێ��o�b�t�@�i�P���ځj
	unsigned char *pix1,			// �摜�f�[�^�ێ��o�b�t�@�i�Q���ځj
	int w,							// �摜��
	int h,							// �摜����
	int start_fr,					// �J�n�t���[���ԍ�
	int seri,						// ������ԃt���[����
	int setseri,					// �Œᖳ���t���[����
	int breakmute,					// �����V�[�������Ԋu�t���[����
	int extendmute,					// �����O�㌟���g���t���[����
	int debug,						// �f�o�b�O���\�����鎞��1
	int idx							// ������ԒʎZ�ԍ�
){
	const int space_sc1  = 5;		// �V�[���`�F���W�Ԃ̍Œ�t���[���Ԋu
	const int space_sc2 = 10;		// �V�[���`�F���W�Ԃ̍Œ�t���[���Ԋu�i�ʒu�㏑���ɂȂ�ꍇ�p�P�j
	const int space_sc3 = 30;		// �V�[���`�F���W�Ԃ̍Œ�t���[���Ԋu�i�ʒu�㏑���ɂȂ�ꍇ�p�Q�j
	const int THRES_RATE1 = 300;	// ���m�ȃV�[���`�F���W����p��臒l
	const int THRES_RATE2 = 7;		// �ω��Ȃ�����p��臒l
	INPUT_INFO &vii = video->get_input_info();
	int n = vii.n;					// �t���[����
	int ncount_sc = 0;				// �V�[���`�F���W���J�E���g

	//--- �V�[���`�F���W��񏉊��� ---
	int msel = 0;					// ���Ԗڂ̃V�[���`�F���W���i0-�j
	int d_max_en[DEF_SCMAX];		// �V�[���`�F���W�̗L����
	int d_max_flagsc[DEF_SCMAX];	// �V�[���`�F���W����t���O
	int d_max_pos[DEF_SCMAX];		// �V�[���`�F���W�n�_�t���[���ԍ�
	int d_max_mvec[DEF_SCMAX];		// �V�[���`�F���W�n�_�������
	int d_maxp_mvec[DEF_SCMAX];		// �V�[���`�F���W�P�t���[���O�������
	int d_maxn_mvec[DEF_SCMAX];		// �V�[���`�F���W�P�t���[���㓮�����
	int d_max_mvec2[DEF_SCMAX];		// �V�[���`�F���W�n�_�C���^�[���[�X�p�������
	int d_maxp_mvec2[DEF_SCMAX];	// �V�[���`�F���W�P�t���[���O�C���^�[���[�X�p�������
	int d_maxn_mvec2[DEF_SCMAX];	// �V�[���`�F���W�P�t���[����C���^�[���[�X�p�������
	int d_max_scrate[DEF_SCMAX];	// �V�[���`�F���W�n�_�̃V�[���`�F���W����l�i0-100�j

	for(int k=0; k<DEF_SCMAX; k++){
		d_max_en[k]     = 0;
		d_max_flagsc[k] = 0;
	}

	//--- �ʃV�[���`�F���W�ʒu���擾 ---
	{
		//--- �ʒu���ݒ� ---
		int range_start_fr = start_fr - extendmute - 1;			// �v�Z�J�n�t���[��
		int valid_start_fr = start_fr - extendmute;				// �����J�n�t���[��
		int range_end_fr   = start_fr + seri + extendmute + 1;	// �v�Z�I���t���[��
		int valid_end_fr   = start_fr + seri + extendmute;		// �����I���t���[��
		if (range_start_fr < 0){
			range_start_fr = 0;
		}
		if (valid_start_fr < 0){
			valid_start_fr = 0;
		}
		if (range_end_fr >= n){
			range_end_fr = n-1;
		}
		if (valid_end_fr >= n){
			valid_end_fr = n-1;
		}

		//--- �O��ʒu���擾 ---
		int last_fr = range_start_fr - 1;
		if (last_fr < 0){
			last_fr = 0;
		}
		video->read_video_y8(last_fr, pix0);

		//--- ���[�J���ϐ� ---
		int local_end_fr;				// �V�[���`�F���W�m��t���[���ʒu
		int local_cntsc;				// �w����ԓ��V�[���`�F���W��
		int pos_lastchange;				// �O��ω��ʒu
		int skip_update;				// �A���t���[���V�[���`�F���W�����p
		int cmvec;						// �C���^�[���[�X�̓����������擾�p
		int cmvec2;						// �C���^�[���[�X�̓������Ȃ����擾�p
		int rate_sc;					// �V�[���`�F���W����l�iflag_sc�̌��ƂȂ�l�j
		int flag_sc;					// �V�[���`�F���W����t���O
		int flag_sc_hold = 0;			// �ێ��V�[���`�F���W����t���O
		int last_cmvec  = 0;			// �O�t���[���̓������L���p
		int last_cmvec2 = 0;			// �O�t���[���̃C���^�[���[�X�p�������L���p
		int last_rate = 0;				// ���O�̃V�[���`�F���W����̈�l
		int keep_schk = 0;				// ���O�V�[���`�F���W�̕ێ��ώ@�t���O
		int keep_msel = 0;				// ���O�V�[���`�F���W�̕ێ��ʒu

		//--- �����J�n�O�ݒ� ---
		local_cntsc = 0;						// �w����ԓ��V�[���`�F���W��
		local_end_fr = start_fr + breakmute;	// ���̃V�[���`�F���W�m��t���[���w��
		pos_lastchange = -space_sc1;			// �O��ω��ʒu
		if (*lastmute_scpos > 0){				// �O��V�[���`�F���W�����݂���ꍇ
			pos_lastchange = *lastmute_scpos;	// �V�[���`�F���W����͊Ԋu�������邽��
		}

		//--- �e�t���[���摜�f�[�^����V�[���`�F���W�����擾 ---
		for (int x=range_start_fr; x<=range_end_fr; x++) {
			//--- �f�[�^�擾 ---
			video->read_video_y8(x, pix1);
			rate_sc = mvec( &cmvec, &cmvec2, &flag_sc, pix1, pix0, w, h, (100-0)*(100/FIELD_PICTURE), FIELD_PICTURE, x);
			if (d_max_en[msel] > 0){
				if (x == d_max_pos[msel]+1){			// �V�[���`�F���W�P�t���[����̓������X�V
					d_maxn_mvec[msel]  = cmvec;
					d_maxn_mvec2[msel] = cmvec2;
				}
			}
			//--- �V�[���`�F���W�i�[���� ---
			if (msel < DEF_SCMAX-1){							// �z�񂪖��܂��Ă��Ȃ����ƑO��
				if (flag_sc_hold > 0){							// �V�[���`�F���W���o�؂�ւ��n�_
					if (local_cntsc < 1){						// ���n�_���c���Ă���Ɍ��ǉ��\�Ȏ�
						local_cntsc ++;
						msel ++;
						flag_sc_hold = 0;
					}
				}
				if (x >= local_end_fr){			// �w����Ԗ������������ꍇ
					if (local_cntsc == 0 && d_max_en[msel] > 0){	// �V�[���`�F���W���Ȃ�������m��
						msel ++;
					}
					else if (flag_sc_hold > 0){		// �V�[���`�F���W�m��
						msel ++;
					}
					else{
						d_max_en[msel] = 0;
					}
					local_end_fr += breakmute;	// ���̃V�[���`�F���W�m��t���[���w��
					local_cntsc = 0;			// �w����ԓ��V�[���`�F���W��
					flag_sc_hold = 0;
				}
			}
			//--- �V�[���`�F���W���菈�� ---
			if (x < valid_start_fr || x > valid_end_fr){		// �����͈͊O
			}
			else{
				//--- �V�[���`�F���W�㏑���ɂȂ�ꍇ�̎��s�𔻕� ---
				if (flag_sc > 0 && d_max_flagsc[msel] > 0){			// �V�[���`�F���W����ŏ㏑���ɂȂ�ꍇ
//					printf("overwrite %d,%d,%d -> %d,%d,%d\n", d_max_pos[msel], d_max_mvec[msel], d_max_scrate[msel], x, cmvec, rate_sc);
					if (abs(x - d_max_pos[msel]) <= space_sc2){ 	// �㏑���O�̈ʒu����Ԋu���Z���ꍇ
						if (d_max_scrate[msel] >= THRES_RATE1 ||	// �O��n�_�̉�ʓ]���������傫���ꍇ������
							rate_sc < THRES_RATE1 ||				// ����n�_�̉�ʓ]���������������ꍇ������
							last_rate * 2 > rate_sc){				// �O��ʂ���ω����傫���Ȃ��ꍇ������
							flag_sc = 0;
						}
					}
					if (abs(x - d_max_pos[msel]) <= space_sc3){		// �㏑���O�̈ʒu����Ԋu���w����̏ꍇ
						if (d_max_scrate[msel] > rate_sc ||			// �O�����ʓ]���������������ꍇ������
							last_rate * 2 > rate_sc){				// �O��ʂ���ω����傫���Ȃ��ꍇ������
							flag_sc = 0;
						}
					}
				}
//if (x > 50108 && x < 50118){
//printf("(%d %d %d %d %d)\n", x, flag_sc, rate_sc, last_rate, pos_lastchange);
//}
				//--- �V�[���`�F���W����̍ăV�[���`�F���W���s�𔻕� ---
				// �����V�[���`�F���W���o����͘A���ŕێ����Ȃ��悤�Ԋu��������
				skip_update = 0;
				if (abs(x - pos_lastchange) <= space_sc1 &&
					x - pos_lastchange != 0){					// �O��V�[���`�F���W�t�߂̃t���[����
					skip_update = 1;							// �A���ŕێ����Ȃ��i�W���ݒ�j
					if (keep_schk > 0){							// �����ւ��\������ώ@�K�v��
						if (flag_sc > 0 &&						// ������V�[���`�F���W
							rate_sc >= THRES_RATE1 &&			// ��ʓ]���������傫��
							d_max_scrate[keep_msel] * 2 <= rate_sc &&	// �O��n�_���͂邩�ɕω���
							last_rate * 3 <= rate_sc){			// �O��ʂ��͂邩�ɕω���
							// ��L�������͗�O�Ƃ��ăV�[���`�F���W�ݒ���s���i�㏑���ݒ�j
							skip_update = 0;
							if (msel != keep_msel){				// �ݒ�ʒu���ς���Ă�����߂�
								msel = keep_msel;
								if (local_cntsc > 0){			// �J�E���g�������Ă�����߂�
									local_cntsc --;
								}
							}
						}
					}
				}
				else{											// �V�[���`�F���W���痣�ꂽ��
					keep_schk = 0;								// �ώ@�I��
				}
				//--- �V�[���`�F���W�X�V���� ---
				if (skip_update == 0){
					int flag_rateup = 0;
					if ((d_max_scrate[msel] < rate_sc && rate_sc >= THRES_RATE2) ||
						(d_max_scrate[msel] == rate_sc && d_max_mvec[msel] < cmvec) ||
						(d_max_scrate[msel] < THRES_RATE2 &&
						 rate_sc < THRES_RATE2 && d_max_mvec[msel] < cmvec)){
						flag_rateup = 1;
					}
					if ((d_max_flagsc[msel] == 0 && flag_rateup > 0)||
							  d_max_en[msel] == 0 || flag_sc > 0) {		// �V�[���`�F���W�n�_�X�V
						d_max_en[msel]     = 1;
						d_max_pos[msel]    = x;
						d_max_mvec[msel]   = cmvec;
						d_maxp_mvec[msel]  = last_cmvec;
						d_maxn_mvec[msel]  = 0;
						d_max_mvec2[msel]  = cmvec2;
						d_maxp_mvec2[msel] = last_cmvec2;
						d_maxn_mvec2[msel] = 0;
						d_max_scrate[msel] = rate_sc;
						if (flag_sc > 0){			// �V�[���`�F���W����
							flag_sc_hold = 1;
							d_max_flagsc[msel] = 1;
							pos_lastchange = x;						// �V�[���`�F���W����͊Ԋu�������邽�߈ʒu�ێ�
							if (rate_sc < THRES_RATE1){				// ��ʓ]���ʂ����Ȃ����͐��t���[���v�ώ@
								keep_schk = 1;
								keep_msel = msel;
							}
						}
					}
				}
			}
			//--- ���̃t���[������ ---
			unsigned char *tmp = pix0;
			pix0 = pix1;
			pix1 = tmp;
			last_cmvec  = cmvec;
			last_cmvec2 = cmvec2;
			last_rate = rate_sc;
//if (x>=48889 && x<=48910) printf("[%d:%d,%d,%d,%d]", x,cmvec,rate_sc,d_max_mvec[msel],d_max_flagsc[msel]);
//					if (x>=9265 && x<=9269){
//						fprintf(fout, "(%d:%d)",x,cmvec);
//					}
		}

		// �Q�ӏ��ڈȍ~�ŃV�[���`�F���W���Ȃ������疳����
		if (flag_sc_hold == 0 && msel > 0){
			if (local_cntsc > 0 || (local_end_fr - breakmute + setseri > range_end_fr)){
				d_max_en[msel] = 0;
			}
		}
	}

	//--- �V�[���`�F���W�����H ---
	int d_maxpre_pos[DEF_SCMAX];		// �V�[���`�F���W�O�̈ʒu
	int d_maxrev_pos[DEF_SCMAX];		// �V�[���`�F���W��̈ʒu
	int msel_max = 0;					// �ő�̃V�[���`�F���W�I��
	{
		// add for searching last frame before changing scene
		// �V�[���`�F���W�O��̃t���[���ԍ����擾�i�C���^�[���[�X�Б��ω������O���j
		for(int k=0; k<=msel; k++){
			if (d_max_en[k] > 0){
				d_maxpre_pos[k] = d_max_pos[k] - 1;		// �ʏ�͂P�t���[���O���V�[���`�F���W�O
				if (d_max_mvec[k] < d_maxp_mvec[k] * 2 && d_maxp_mvec[k] > d_maxp_mvec2[k] * 2 &&
					d_max_mvec[k] - d_max_mvec2[k] > d_max_mvec[k] / 16){
					d_maxpre_pos[k] = d_max_pos[k] - 2;
				}
				d_maxrev_pos[k] = d_max_pos[k];			// �ʏ�̓V�[���`�F���W�n�_���V�[���`�F���W��
				if (d_maxrev_pos[k] - d_maxpre_pos[k] < 2){
					if (d_max_mvec[k] > d_max_mvec2[k] * 2 &&
						d_max_mvec[k] < d_maxn_mvec[k] * 2 &&
						d_maxn_mvec[k] - d_maxn_mvec2[k] > d_maxn_mvec[k] / 16 &&
						(d_maxn_mvec[k] > d_maxn_mvec2[k] * 2 || d_max_mvec[k] < d_maxn_mvec2[k] * 2)){
						d_maxrev_pos[k] = d_max_pos[k] + 1;
					}
				}
				if (d_maxpre_pos[k] < 0){
					d_maxpre_pos[k] = 0;
				}
				if (d_maxrev_pos[k] < 0){
					d_maxrev_pos[k] = 0;
				}
			}
		}
		// ������₪����A���ω��̂Ȃ��V�[���`�F���W������ꍇ�͍폜
		if (msel >= 1){
			// �ő�ω��ʒu�m�F�imax_msel�l���X�V�j
			int d_tmpmax      = -1;
			int rate_tmpmax   = -1;
			for(int k=0; k<=msel; k++){
				if (d_max_en[k] > 0){
					if ((d_max_scrate[k] > rate_tmpmax) ||
						(d_max_scrate[k] == rate_tmpmax && d_max_mvec[k] > d_tmpmax)){
						msel_max = k;
						rate_tmpmax = d_max_scrate[k];
						d_tmpmax    = d_max_mvec[k];
					}
				}
			}
			// �ő�ω��ʒu�������A�ω��̂Ȃ����͍폜
			for(int k=0; k<=msel; k++){
				if (d_max_en[k] > 0){
					if (d_max_scrate[k] < THRES_RATE2 && k != msel_max){
						d_max_en[k] = 0;
					}
				}
			}
		}

		// �Ō�̃V�[���`�F���W�ʒu���L���i���̖�����Ԃ̃I�[�o�[���b�v���o�p�j
		int tmp_scpos = -1;
		int tmp_flagsc = 0;
		for(int k=0; k<=msel; k++){
			if (d_max_en[k] > 0){
				if (tmp_scpos < d_max_pos[k]){
					tmp_scpos  = d_max_pos[k];
					tmp_flagsc = d_max_flagsc[k];
				}
			}
		}
		if (tmp_flagsc > 0){			// �Ōオ���m�ȃV�[���`�F���W���������̂ݐݒ�
			*lastmute_scpos = tmp_scpos;
		}
		else{
			*lastmute_scpos = -1;
		}
	}

	//--- �V�[���`�F���W���\���p ---
	{
		// �����Ԗ����V�[���`�F���W�ݒ蕝�����邩�m�F
		int flag_force_sc = 0;			// �w����ԓ��ŋ����I�ɃV�[���`�F���W���s������
		if (msel > 0){		// �Q�ȏ�̊Ԋu
			int msel_s = -1;			// �ŏ��̗L���ԍ�
			int msel_e = -1;			// �Ō�̗L���ԍ�
			for(int k=0; k<=msel; k++){
				if (d_max_en[k] > 0){
					msel_e = k;
					if (msel_s < 0){
						msel_s = k;
					}
				}
			}
			if (d_max_pos[msel_e] - d_max_pos[msel_s] > breakmute){
				flag_force_sc = 1;
			}
		}

		// �}�[�N���e�ƈʒu�����߂�
		int msel_mark = msel_max;
		int msel_mknext  = msel_max;
		int mark_type = 0;
		int difmin = 30;
		int last_frame;
		if (*lastmute_marker < 0){					// �ŏ��̖����̑O��͖���
			last_frame = -10000;
		}
		else{
			last_frame = *lastmute_marker;
		}
		for(int k=0; k<=msel; k++){
			if (d_max_en[k] == 0) continue;		// �V�[���`�F���W��₩��O�ꂽ�ꍇ����
			int dif = abs(d_max_pos[k] - last_frame - 30*15);
			if (dif < difmin){
				difmin = dif;
				msel_mark = k;
				mark_type = 15;				// 15�b�Ԋu
			}
			dif = abs(d_max_pos[k] - last_frame - 30*30);
			if (dif < difmin){
				difmin = dif;
				msel_mark = k;
				mark_type = 30;				// 30�b�Ԋu
			}
			dif = abs(d_max_pos[k] - last_frame - 30*45);
			if (dif < difmin){
				difmin = dif;
				msel_mark = k;
				mark_type = 45;				// 45�b�Ԋu
			}
			dif = abs(d_max_pos[k] - last_frame - 30*60);
			if (dif < difmin){
				difmin = dif;
				msel_mark = k;
				mark_type = 60;				// 60�b�Ԋu
			}
			if (k == msel_mark){			// �}�[�N�ʒu�X�V��
				msel_mknext = k;			// ���}�[�N�N�_�����l�ɍX�V
			}
			if (seri > breakmute){			// �������Ԃ������ꍇ
				if (k > msel_mark){			// �}�[�N���痣��Ă���ꍇ
					if (abs(d_max_pos[k] - d_max_pos[msel_mark]) > breakmute){
						msel_mknext = k;	// ���}�[�N�N�_�͍Ō�̈ʒu
					}
				}
			}
		}

		//--- ���ʕ\�� ---
		for(int k=0; k<=msel; k++){
			char *mark = "";
			if (d_max_en[k] == 0) continue;		// �V�[���`�F���W��₩��O�ꂽ�ꍇ����

			if (d_max_scrate[k] < THRES_RATE2){	// �S���ω��̂Ȃ��V�[���`�F���W
				mark = "�Q";
			}
			else if ((flag_force_sc > 0 && (k != msel_mark || mark_type == 0)) ||	// �w�薳����ԓ��V�[���`�F���W�n�_
					  (d_max_scrate[k] < THRES_RATE2 && k != msel_max)){			// �����Ȃ��Ŏc���Ă���ꍇ
				mark = "��";
			}
			else if (k == msel_mark){
				if (idx > 1 && mark_type == 15) {
					mark = "��";
				} else if (idx > 1 && mark_type == 30) {
					mark = "����";
				} else if (idx > 1 && mark_type == 45) {
					mark = "������";
				} else if (idx > 1 && mark_type == 60) {
					mark = "��������";
				}
			}
			else{	// ������ԓ��ő�2���V�[���`�F���W
					mark = "��";
			}
			printf("\t SCPos: %d %s\n", d_max_pos[k], mark);
			ncount_sc ++;

			TCHAR title[256];
			sprintf_s(title, _T("%d�t���[�� %s SCPos:%d %d"), seri, mark, d_maxrev_pos[k], d_maxpre_pos[k]);
			if (debug == 0){		// normal
				write_chapter(fout, idx, start_fr, title, &vii);
			}
			else{					// for debug
				TCHAR tmp_title[256];
				sprintf_s(tmp_title, _T(" Rate:%d"), d_max_scrate[k]);
				strcat(title, tmp_title);
//				sprintf_s(tmp_title, _T(" : [%d %d] [%d %d] [%d %d]"), d_maxn_mvec[k], d_maxn_mvec2[k], d_max_mvec[k], d_max_mvec2[k], d_maxp_mvec[k], d_maxp_mvec2[k]);
//				strcat(title, tmp_title);
				write_chapter_debug(fout, idx, start_fr, title, &vii);
			}
		}
		*lastmute_marker = d_max_pos[msel_mknext];
	}
	return ncount_sc;			// ���o�����ʒu�̍��v��Ԃ�
}
