#pragma once

#include "stdafx.h"

// FAW�`�F�b�N�ƁAFAWPreview.auf���g���Ă�1�t���[���f�R�[�h
class CFAW {
	bool is_half;

	bool load_failed;
	HMODULE _h;

	typedef int (__stdcall *ExtractDecode1FAW)(const short *in, int samples, short *out, bool is_half);
	ExtractDecode1FAW _ExtractDecode1FAW;

	bool load() {
		if (_ExtractDecode1FAW == NULL && load_failed == false) {
			_h = LoadLibrary("FAWPreview.auf");
			if (_h == NULL) {
				load_failed = true;
				return false;
			}
			_ExtractDecode1FAW = (ExtractDecode1FAW)GetProcAddress(_h, "ExtractDecode1FAW");
			if (_ExtractDecode1FAW == NULL) {
				FreeLibrary(_h);
				_h = NULL;
				load_failed = true;
				return false;
			}
			return true;
		}
		return _ExtractDecode1FAW != NULL;
	}
public:
	CFAW() : _h(NULL), _ExtractDecode1FAW(NULL), load_failed(false), is_half(false) { }

	~CFAW() {
		if (_h) {
			FreeLibrary(_h);
		}
	}

	bool isLoadFailed(void) {
		return load_failed;
	}

	// FAW�J�n�n�_��T���B1/2��FAW��������΁A�ȍ~�͂��ꂵ���T���Ȃ��B
	// in: get_audio()�œ��������f�[�^
	// samples: get_audio() * ch��
	// �߂�l�FFAW�J�n�ʒu�̃C���f�b�N�X�B�Ȃ����-1
	int findFAW(short *in, int samples) {
		// search for 72 F8 1F 4E 07 01 00 00
		static unsigned char faw11[] = {0x72, 0xF8, 0x1F, 0x4E, 0x07, 0x01, 0x00, 0x00};
		if (is_half == false) {
			for (int j=0; j<samples - 30; ++j) {
				if (memcmp(in+j, faw11, sizeof(faw11)) == 0) {
					return j;
				}
			}
		}

		// search for 00 F2 00 78 00 9F 00 CE 00 87 00 81 00 80 00 80
		static unsigned char faw12[] = {0x00, 0xF2, 0x00, 0x78, 0x00, 0x9F, 0x00, 0xCE,
										0x00, 0x87, 0x00, 0x81, 0x00, 0x80, 0x00, 0x80};

		for (int j=0; j<samples - 30; ++j) {
			if (memcmp(in+j, faw12, sizeof(faw12)) == 0) {
				is_half = true;
				return j;
			}
		}

		return -1;
	}

	// FAWPreview.auf���g����FAW�f�[�^1�𒊏o���f�R�[�h����
	// in: FAW�J�n�ʒu�̃|�C���^�BfindFAW�ɓn����in + findFAW�̖߂�l
	// samples: in�ɂ���f�[�^��short���Z�ł̃T�C�Y
	// out: �f�R�[�h���ʂ�����o�b�t�@(16bit, 2ch��1024�T���v��)
	//     �i1024sample * 2byte * 2ch = 4096�o�C�g�K�v�j
	int decodeFAW(const short *in, int samples, short *out){
		if (load()) {
			return _ExtractDecode1FAW(in, samples, out, is_half);
		}
		return 0;
	}
};

// FAW�f�R�[�h�t�B���^
class FAWDecoder : public NullSource {
	CFAW _cfaw;
	Source *_src;
	WAVEFORMATEX fmt;
public:
	FAWDecoder(Source *src) : NullSource(), _src(src){
		ZeroMemory(&fmt, sizeof(fmt));
		fmt.wFormatTag = WAVE_FORMAT_PCM;		
		fmt.nChannels = 2;
		fmt.nSamplesPerSec = 48000;
		fmt.wBitsPerSample = 16;
		fmt.nBlockAlign = fmt.wBitsPerSample / 8 * fmt.nChannels;
		fmt.nAvgBytesPerSec = fmt.nBlockAlign * fmt.nSamplesPerSec;
		fmt.cbSize = 0;
		_ip.audio_format = &fmt;
		_ip.audio_format_size = sizeof(fmt);
		_ip.audio_n = -1;

		_ip.flag = INPUT_INFO_FLAG_AUDIO;
	}

	int release() {
		_src->release();
		return NullSource::release();
	}

	int read_audio(int frame, short *buf) {
		int nsamples = _src->read_audio(frame, buf);
		nsamples *= _src->get_input_info().audio_format->nChannels;

		int j = _cfaw.findFAW(buf, nsamples);
		if (j == -1) {
			return 0;
		}

		// 2ch�Ȃ̂�2�Ŋ���
		return _cfaw.decodeFAW(buf+j, nsamples-j, buf) / 2;
	}
};
