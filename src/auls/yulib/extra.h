////////////////////////////////////////////////////////////////////////////////
// yulib/extra.h
////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef YULIB_EXTRA_H
#define YULIB_EXTRA_H

#include <stdio.h>
#include <windows.h>
#include <imagehlp.h>

#pragma comment(lib, "imagehlp.lib")

namespace yulib {\

////////////////////////////////////////////////////////////////////////////////
// �֐����������֐�
// 
// �Q�l�T�C�g
// http://qiita.com/kobake@github/items/8d3d3637c7af0b270098
////////////////////////////////////////////////////////////////////////////////

void* RewriteFunction(LPCSTR modname, LPCSTR funcname, void* funcptr)
{
	// �x�[�X�A�h���X
	DWORD_PTR dwBase = (DWORD_PTR)GetModuleHandle(modname);
	if(!dwBase) return NULL;

	// �C���[�W��
	ULONG ulSize;
	PIMAGE_IMPORT_DESCRIPTOR pImgDesc = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToData((HMODULE)(intptr_t)dwBase, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &ulSize);
	for(; pImgDesc->Name; pImgDesc++){
		//const char* szModuleName = (char*)(intptr_t)(dwBase+pImgDesc->Name);
		// THUNK���
		PIMAGE_THUNK_DATA pFirstThunk = (PIMAGE_THUNK_DATA)(intptr_t)(dwBase+pImgDesc->FirstThunk);
		PIMAGE_THUNK_DATA pOrgFirstThunk = (PIMAGE_THUNK_DATA)(intptr_t)(dwBase+pImgDesc->OriginalFirstThunk);
		// �֐���
		for(;pFirstThunk->u1.Function; pFirstThunk++, pOrgFirstThunk++){
			if(IMAGE_SNAP_BY_ORDINAL(pOrgFirstThunk->u1.Ordinal))continue;
			PIMAGE_IMPORT_BY_NAME pImportName = (PIMAGE_IMPORT_BY_NAME)(intptr_t)(dwBase+(DWORD_PTR)pOrgFirstThunk->u1.AddressOfData);

			// ������������
			if(_stricmp((const char*)pImportName->Name, funcname) != 0)continue;

			// �ی��ԕύX
			DWORD dwOldProtect;
			if( !VirtualProtect(&pFirstThunk->u1.Function, sizeof(pFirstThunk->u1.Function), PAGE_READWRITE, &dwOldProtect) )
				return NULL; // �G���[

			// ��������
			void* pOrgFunc = (void*)(intptr_t)pFirstThunk->u1.Function; // ���̃A�h���X��ۑ����Ă���
			WriteProcessMemory(GetCurrentProcess(), &pFirstThunk->u1.Function, &funcptr, sizeof(pFirstThunk->u1.Function), NULL);
			pFirstThunk->u1.Function = (DWORD_PTR)(intptr_t)funcptr;

			// �ی��Ԗ߂�
			VirtualProtect(&pFirstThunk->u1.Function, sizeof(pFirstThunk->u1.Function), dwOldProtect, &dwOldProtect);
			return pOrgFunc; // ���̃A�h���X��Ԃ�
		}
	}
	return NULL;
}

} // namespace yulib

#endif // #ifndef YULIB_EXTRA_H
