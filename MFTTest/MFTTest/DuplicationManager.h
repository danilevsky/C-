#pragma once
#ifndef _DUPLICATIONMANAGER_H_
#define _DUPLICATIONMANAGER_H_

#include "CommonTypes.h"  

//
// Capture screen class
//
class DuplicationManager
{
public:
	DuplicationManager();
	~DuplicationManager();
	_Success_(*Timeout == false && return == DUPL_RETURN_SUCCESS) HRESULT GetFrame(_Out_ FRAME_DATA* Data); 
	DUPL_RETURN InitDupl(UINT Output);
	DUPL_RETURN DoneWithFrame();
	ID3D11Device* GetID3D11Device();

private:
	IDXGIOutputDuplication * m_DeskDupl;
	ID3D11Texture2D* m_AcquiredDesktopImage;
	_Field_size_bytes_(m_MetaDataSize) BYTE* m_MetaDataBuffer;
	UINT m_MetaDataSize;
	UINT m_OutputNumber;
	DXGI_OUTPUT_DESC m_OutputDesc;
	ID3D11DeviceContext* Context;
	ID3D11Device * m_Device;
};

#endif