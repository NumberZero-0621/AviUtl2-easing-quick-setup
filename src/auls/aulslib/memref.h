////////////////////////////////////////////////////////////////////////////////
// aulslib/memref.h
// �������Q�Ɨp�w�b�_
////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef AULSLIB_MEMREF_H
#define AULSLIB_MEMREF_H

#include "aviutl.h"
#include "exedit.h"

struct lua_State;

namespace auls {\

static const LPCSTR MEMREF_FILTER_NAME     = TEXT("�������Q��");
static const LPCSTR MEMREF_PLUGIN_FILENAME = "auls_memref.auf";

static const int MEMREF_ERRORCODE_GET_MEMORY_FAILED = -1;

struct EXEDIT_OBJECT;
struct EXEDIT_FILTER;
struct CAMERA_ZBUFFER;
struct SCENE_SETTING;
struct LAYER_SETTING;

#define AULS_MEMREF_DEFLIST\
	def(EXEDIT_FILTER**,  StaticFilterTable, 0x0A3E28)\
	def(int*,             SortedObjectTable_LayerIndexEnd, 0x135AC8)\
	def(LPSTR,            AliasNameBuffer, 0x135C70)\
	def(int*,             ObjDlg_CommandTarget, 0x14965C)\
	def(int*,             SortedObjectTable_LayerIndexBegin, 0x149670)\
	def(BYTE*,            ObjDlg_FilterStatus, 0x158D38)\
	def(EXEDIT_OBJECT**,  SortedObjectTable, 0x168FA8)\
	def(int*,                ObjDlg_ObjectIndex, 0x177A10)\
	def(SCENE_SETTING*,      SceneSetting, 0x177A50)\
	def(EXEDIT_FILTER**,     LoadedFilterTable, 0x187C98)\
	def(LAYER_SETTING*,      LayerSetting, 0x188498)\
	def(int*,                SceneDisplaying, 0x1A5310)\
	def(wchar_t*,            TextBuffer, 0x1A6BD0)\
	def(int*,                TraScript_ProcessingTrackBarIndex, 0x1B21F4)\
	def(int*,                TraScript_ProcessingObjectIndex, 0x1B2B04)\
	def(EXEDIT_FILTER*,      ScriptProcessingFilter, 0x1B2B10)\
	def(lua_State*,          LuaState, 0x1BACA8)\
	def(OBJECT_BUFFER_INFO*, ObjectBufferInfo, 0x1E0F9C)\
	def(CAMERA_ZBUFFER**,    CameraZBuffer, 0x1EC7AC)\
	def(UNDO_INFO*,          UndoInfo, 0x244E08)

// �������Q�ƃN���X
class CMemref {
public:
	HMODULE module;
	int     error;

	bool Init(FILTER *fp)
	{
		// ���W���[���n���h�����擾
		if(fp) {
			FILTER *memref = AviUtl_GetFilter(fp, MEMREF_FILTER_NAME);
			module = (HMODULE)memref->dll_hinst;
		} else {
			module = GetModuleHandle(MEMREF_PLUGIN_FILENAME);
		}
		if(!module) return false;

		int(*f)(FILTER*) = (int(*)(FILTER*))GetProcAddress(module, "Init");
		if(!f) return false;
		error = f(fp);
		return error == 0;
	}

	LPCSTR GetErrorString(void)
	{
		if(error == MEMREF_ERRORCODE_GET_MEMORY_FAILED) {
			return "[�������Q�ƃv���O�C��]\r\n�������A�h���X�̎擾�Ɏ��s���܂����B";
		} else if(module) {
			LPCSTR (*f)(int) = (LPCSTR(*)(int))GetProcAddress(module, "GetErrorString");
			if(f) {
				LPCSTR str = f(error);
				if(str) return str;
			}
			return "[�������Q�ƃv���O�C��]\r\n�G���[�����擾�ł��܂���ł����B";
		} else {
			return "�������Q�ƃv���O�C����������܂���ł����B";
		}
	}

#define def(type, name, offset)\
	type Exedit_##name(void)\
	{\
		if(!module) return NULL;\
		type(*f)(void) = (type(*)(void))GetProcAddress(module, "Exedit_" #name);\
		if(!f) {\
			error = MEMREF_ERRORCODE_GET_MEMORY_FAILED;\
			return NULL;\
		}\
		return f();\
	}

	AULS_MEMREF_DEFLIST
#undef def
};

} // namespace auls

#endif // #ifndef AULSLIB_MEMREF_H
