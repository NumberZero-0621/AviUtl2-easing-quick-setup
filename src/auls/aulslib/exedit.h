////////////////////////////////////////////////////////////////////////////////
// aulslib/exedit.h
////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef AULSLIB_EXEDIT_H
#define AULSLIB_EXEDIT_H

#include "../yulib/color.h"

#include "aviutl.h"

namespace auls {\

static const LPCSTR EXEDIT_NAME = "�g���ҏW";
static const LPCSTR OBJDLG_CLASSNAME = "ExtendedFilterClass";

static const int EXEDIT_FILTER_ID_BEGIN = 2000;
static const int EXEDIT_ALIAS_ID_BEGIN  = 3000;

// �ݒ�_�C�A���O�R�}���hID
struct OBJDLG_COMMAND {
	static const DWORD FILTER_SET_DEFVALUE = 1108;
	static const DWORD FILTER_MAKE_ALIAS   = 1109;
	static const DWORD FILTER_MOVEUP   = 1116;
	static const DWORD FILTER_MOVEDOWN = 1117;
	static const DWORD FILTER_DELETE   = 4300;
	static const DWORD FILTER_INIT     = 1105;
	static const DWORD FILTER_VALIDATE = 4400;
	static const DWORD FILTER_FOLD     = 4500;
};

inline FILTER *Exedit_GetFilter(FILTER *fp)
{
	for(int i = AviUtl_GetFilterNumber(fp); i;) {
		FILTER *exedit = (FILTER*)fp->exfunc->get_filterp(--i);
		if(!strcmp(exedit->name, EXEDIT_NAME)) return exedit;
	}
	return NULL;
}

inline HWND Exedit_GetWindow(FILTER *fp)
{
	FILTER *exedit = Exedit_GetFilter(fp);
	return exedit ? exedit->hwnd : NULL;
}

// �I�u�W�F�N�g�ݒ�E�B���h�E���擾
//  ���������Y�E�B���h�E���쐬�����̂� WM_FILTER_INIT ���_�Ȃ̂ŁA
//  �v���O�C���̏��������̓s���A�m���Ɏ擾�ł���̂� WM_FILTER_CHANGE_WINDOW �Ȍ�ł���B
inline HWND ObjDlg_GetWindow(HWND hwnd_exedit)
{
	struct local {
		HWND exedit;
		HWND objdlg;

		static BOOL CALLBACK enumproc(HWND hwnd, LPARAM lp) {
			local *p = (local*)lp;
			HWND owner = GetWindow(hwnd, GW_OWNER);
			if(owner == p->exedit) {
				p->objdlg = hwnd;
				return FALSE;
			} else {
				return TRUE;
			}
		}
	} l = { hwnd_exedit, NULL };

	DWORD id = GetCurrentThreadId();
	EnumThreadWindows(id, local::enumproc, (LPARAM)&l);
	return l.objdlg;
}



struct EXEDIT_OBJECT {
	// �萔�錾
	static const BYTE FLAG_CLIPPING = 0x01;
	static const BYTE FLAG_CAMERA   = 0x02;

	/*
	static const BYTE TYPE_IMAGE_FILTER = 0;
	static const BYTE TYPE_IMAGE_OBJECT = 1;
	static const BYTE TYPE_SOUND_FILTER = 2;
	static const BYTE TYPE_SOUND_OBJECT = 3;
	static const BYTE TYPE_IMAGE_EFFECT = 4;
	static const BYTE TYPE_SOUND_EFFECT = 6; // �ʏ�A�P�̂Ŕz�u�͂ł��Ȃ�
	static const BYTE TYPE_CAMERA_TIME_CONTROL  = 0x18;
	static const BYTE TYPE_GROUP_CONTROL = 0x1C;*/
	static const BYTE TYPE_OVERLAY   = 0x01; 
	static const BYTE TYPE_SOUND     = 0x02;
	static const BYTE TYPE_EFFECT    = 0x04;
	static const BYTE TYPE_LIGHTBLUE   = 0x08; // �O���[�v����A�J��������(�J�����G�t�F�N�g�܂�)�A���Ԑ���
	static const BYTE TYPE_CONTROL = 0x10; // �Ώۃ��C���[�͈͂��^�C�����C����ɕ\�������

	static const size_t MAX_DISPNAME = 64;
	static const int    MAX_FILTER = 12;
	static const int    MAX_TRACK = 64;
	static const int    MAX_CHECK = 48;

	static const BYTE FILTER_STATUS_VALID = 0x01;
	static const BYTE FILTER_STATUS_FOLD = 0x02;
	static const BYTE FILTER_STATUS_GUI_VALID = 0x04;

	struct FILTER_PARAM {
		static const int INVALID_ID = -1;

		int   id;
		short track_begin; // ���̃t�B���^�̐擪�̃g���b�N�o�[�ԍ�
		short check_begin;
		DWORD exdata_offset;
	};

	struct TRACK_MODE {
		static const short NUM_USE_SCRIPT = 0x0F;
		static const short NUM_DECELERATE = 0x20;
		static const short NUM_ACCELERATE = 0x40;

		short num;
		short script_num;

		bool Accelerate(void) { return (num & NUM_ACCELERATE) != 0; }
		bool Decelerate(void) { return (num & NUM_DECELERATE) != 0; }
		bool UseScript(void)  { return (num & NUM_USE_SCRIPT) == NUM_USE_SCRIPT; }
	};

	// �ϐ��錾
	BYTE exists;
	BYTE flag; // 1:��̃I�u�W�F�N�g�ŃN���b�s���O 2:�J��������̑Ώ�
	BYTE type; // 
	BYTE padding_;
	int  layer_disp; // �\�����C���[�A�ʃV�[���\������-1
	int  frame_begin;
	int  frame_end;
	char dispname[MAX_DISPNAME]; // �^�C�����C����̕\����(�ŏ��̃o�C�g���k�������̏ꍇ�̓f�t�H���g�����\��)
	int  index_midpt_leader; // ���ԓ_�����I�u�W�F�N�g�̍\���v�f�̏ꍇ�A�擪�I�u�W�F�N�g�̃C���f�b�N�X�A���ԓ_�������Ȃ��Ȃ�-1
	FILTER_PARAM filter_param[MAX_FILTER];
	BYTE filter_status[MAX_FILTER];
	short track_sum;
	short check_sum;
	DWORD exdata_sum;
	int   track_value_left[MAX_TRACK];
	int   track_value_right[MAX_TRACK];
	TRACK_MODE track_mode[MAX_TRACK];
	int   check_value[MAX_CHECK];
	DWORD exdata_offset;
	int   group_belong;
	int   track_param[MAX_TRACK];
	int   layer_set;
	int   scene_set;

	// �֐���`
	int GetFilterNum(void)
	{
		for(int i = 0; i < MAX_FILTER; i++) {
			if(filter_param[i].id == FILTER_PARAM::INVALID_ID) return i;
		}
		return MAX_FILTER;
	}

	DWORD ExdataOffset(int idx)
	{
		return exdata_offset + filter_param[idx].exdata_offset;
	}
};

struct EXEDIT_FILTER {
	static const DWORD FLAG_INPUT_FILTER   = 0x00000008;
	static const DWORD FLAG_OUTPUT_FILTER  = 0x00000010;
	static const DWORD FLAG_EFFECT         = 0x00000020;
	static const DWORD FLAG_DISABLE_PUT    = 0x00000080; // �^�C�����C���z�u���j���[�ɂ͏o�Ă��Ȃ�
	static const DWORD FLAG_DISABLE_ADDING = 0x00000100; // �^�C�����C���z�u���j���[�ɂ̂ݏo���A�G�t�F�N�g�ǉ����j���[�ɂ͏o�Ă��Ȃ�
	static const DWORD FLAG_UNKNOWN1       = 0x00000200; // ������O���ƃ}�X�N�A�f�B�X�v���C�X�����g�}�b�v�ŃV�[�����g���Ȃ��Ȃ�B�g���F�ݒ�ŐF���ω����Ȃ��Ȃ�B
	static const DWORD FLAG_BASIC_EFFECT   = 0x00008000;
	static const DWORD FLAG_SOUND_FILTER   = 0x00200000;
	static const DWORD FLAG_CONTROL_FILTER = 0x01000000;
	static const DWORD FLAG_UNKNOWN_RUNTIME = 0x04000000; // ���s���ɂ��ׂẴt�B���^�ɐݒ肳��Ă���(�����������t���O�H)
	static const DWORD FLAG_UNKNOWN        = 0x40000000;

	// �g���f�[�^����(�{�^����h���b�v�_�E�����X�g��)�̃f�[�^
	struct EXDATA_USE {
		static const WORD TYPE_UNKNOWN = 0; // name �� NULL
		static const WORD TYPE_NUMBER  = 1;
		static const WORD TYPE_STRING  = 2;
		static const WORD TYPE_BYTEARRAY = 3;

		WORD type;
		WORD size;
		LPCSTR name; // ���ږ�
	};

	struct TRACK_EXTRA {
		int *track_scale;
		int *track_link;
		int *track_drag_min;
		int *track_drag_max;
	};

	// ���C���E�B���h�E�̓_���g�̕\���ɕK�v�ȍ��ڂɑΉ����Ă���g���b�N�o�[�ԍ�
	struct TRACK_GUI {
		static const int INVALID = -1;

		int bx, by, bz; // ����W
		int rx, ry, rz; // ��]
		int cx, cy, cz; // ���S���W
		int zoom, aspect, alpha;
	};

	DWORD flag;
	int   x; // 0x10000=�o�̓t�B���^�Ȃ�ύX���ɃJ��������Ώۃt���O��on(�O���[�v����ɂ����Ă���)
	int   y; // �f�t�H���g����(���Ȃ�t���[�����A���Ȃ�~���b�A�[���Ȃ�^�C�����C���g�嗦�ɉ���������)
	LPCSTR name;
	int    track_num; // 0x10
	LPCSTR *track_name;
	int    *track_def;
	int    *track_min;
	int    *track_max; // 0x20
	int     check_num;
	LPCSTR *check_name;
	int    *check_def; // -1:�{�^���� -2:�h���b�v�_�E�����X�g��
	void   *func_proc; // 0x30
	void   *func_init;
	void   *func_exit;
	void   *func_update;
	void   *func_WndProc; // 0x40
	void   *track_value;
	int    *check_value;
	void   *exdata_ptr;
	DWORD   exdata_size; // 0x50
	void   *information; // ��� NULL
	void   *func_save_start;
	void   *func_save_end;
	EXFUNC *aviutl_exfunc; // 0x60
	void   *exedit_exfunc;
	void   *dll_inst; // ���NULL
	void   *exdata_def; // 0x6C : �g���f�[�^�̏����l
	EXDATA_USE *exdata_use; // �g���f�[�^���ڂ̖��O�Ɋւ���f�[�^
	TRACK_EXTRA *track_extra; // track_scale, ?, track_drag_min, track_drag_max
	TRACK_GUI   *track_gui;
	int     unknown[20];
	int    *track_scale; // 0xCC : �����_��1�ʂ܂łȂ�10�A��2�ʂ܂łȂ�100�A�����_�����Ȃ�0
	void   *track_link;
	int    *track_drag_min;
	int    *track_drag_max;
	FILTER *exedit_filter;  // �g���ҏW�̃t�B���^�\����
	EXEDIT_OBJECT *object_data; // 0xE0
	short  object_index_processing; // �������I�u�W�F�N�g�̃C���f�b�N�X
	short  filter_pos_processing; // �������I�u�W�F�N�g�̃t�B���^�ʒu
	short  object_index_objdlg; // ��Ɠ����H
	short  filter_pos_objdlg;
	int    frame_start; // �I�u�W�F�N�g�̊J�n�t���[��
	int    frame_end;   // 0xF0 : �I�u�W�F�N�g�̏I���t���[��
	int    *track_value_left; // 
	int    *track_value_right;
	int    *track_mode;
	int    *check_value_; // 0x100
	void   *exdata_;
	int    *track_param;
	void   *offset_10C;
	void   *offset_110;
	void   *offset_114;
	int     frame_start_chain; // 0x118 : ���ԓ_�I�u�W�F�N�g�S�̂̊J�n�t���[��
	int     frame_end_chain; // 0x11C ���I���t���[��
	int     layer_set; // �z�u���C���[
	int     scene_set; // �z�u�V�[��

	bool IsScreenFilter(void)
	{
		return (flag & (FLAG_INPUT_FILTER | FLAG_OUTPUT_FILTER | FLAG_EFFECT | FLAG_CONTROL_FILTER)) == 0;
	}

	int GetExdataUseNum(void)
	{
		if(exdata_size == 0) return 0;
		int num = 0;
		DWORD sum = 0;
		while(sum < exdata_size) {
			sum += exdata_use[num].size;
			num++;
		}
		return num;
	}
};

struct OBJECT_BUFFER_INFO {
	DWORD exdata_size;
	DWORD max_data_num;
	EXEDIT_OBJECT *data;
	void* exdata;
};

struct LAYER_SETTING {
	// �萔
	static const DWORD FLAG_UNDISP = 1;
	static const DWORD FLAG_LOCKED = 2;
	static const DWORD FLAG_COORDLINK = 0x10;
	static const DWORD FLAG_CLIP = 0x20;

	// �ϐ�
	DWORD flag;
	LPCSTR name;
};

struct SCENE_SETTING {
	static const DWORD FLAG_DISPED = 1;
	static const DWORD FLAG_ALPHA  = 2;

	DWORD flag;
	LPCSTR name;
	int width;
	int height;
	int max_frame;
	int current_frame;
	int timeline_scale; // 
	int timeline_disp_begin_pos;
	int selected_object;
	int selected_frame_begin;
	int selected_frame_end;
	BOOL disp_bpm_grid;
	int bpm_grid_tempo; // �\����10000�{�����l
	int bpm_grid_base;
	BOOL disp_xy_grid;
	int xy_grid_width;
	int xy_grid_height;
	BOOL disp_camera_grid;
	int camera_grid_size;
	int camera_grid_num;
	BOOL disp_out_of_frame;
	int out_of_frame_scale; // �t���[���O�̈�̊g�嗦�A12�œ��{�B�傫���قǏk�������
	int bpm_grid_beat;
	int disp_begin_layer;
};

struct CAMERA_ZBUFFER {
	static const DWORD DISTANCE_BASE = 0x77359400; // �J�����Ƃ̋����� 0 �̎��̒l�B1 ������ 0x1000 ����
	static const DWORD ROTATE_MAX = 0x4000;

	DWORD distance;
	DWORD rotate; // XY����]��������ƕω�����B 0x4000 ���ő�H
	DWORD distance_overlapped;
	yulib::ColorYCbCrA color;

	int GetDistance(void)
	{
		return (DISTANCE_BASE - distance) / 0x1000;
	}

	int GetDistanceOverlapped(void)
	{
		return (DISTANCE_BASE - distance) / 0x1000;
	}
};

struct UNDO_INFO {
	int   object_num;
	void* buffer_ptr;
	DWORD write_offset;
	int   current_id;
	DWORD buffer_size;
};

struct UNDO_DATA_HEADER {
	int   data_id;   // �A���h�D�f�[�^�̎��ʎq�B���̒l���ő�̂��̂�����s�����B
	int   object_id; // �ΏۃI�u�W�F�N�g��ID
	DWORD data_size; // �w�b�_�����܂񂾃f�[�^�T�C�Y
	void  *data;

	UNDO_DATA_HEADER *Next(void) { return (UNDO_DATA_HEADER*)((DWORD)this + data_size); }
};

struct CACHE_BUFFER {
	static const DWORD FLAG_IMAGE_FILE = 0x20; // �摜�t�@�C���Aobj.copybuffer("image:xxxx")
	static const DWORD FLAG_IMAGE_COPY = 0x40; // ���[�V�����u���[�Aobj.copybuffer("cache:xxxx")

	int width;
	int height;
	DWORD flag;
	yulib::ColorYCbCrA *data;
	char path[260]; // obj.copybuffer �� "cache:xxxx" �Ƃ������O��n�����ꍇ
	                // *lua_xxxx �Ƃ��Ċi�[�����
};




} // namespace auls

#endif // #ifndef AULSLIB_EXEDIT_H
