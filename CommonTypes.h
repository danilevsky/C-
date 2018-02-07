#pragma once
#include <stdio.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <mfobjects.h>

#define CHECK_HR(hr, msg) if (hr != S_OK) { printf(msg); printf("Error: %.2X.\n", hr); goto done; }

typedef _Return_type_success_(return == DUPL_RETURN_SUCCESS) enum
{
	DUPL_RETURN_SUCCESS = 0,
	DUPL_RETURN_ERROR_EXPECTED = 1,
	DUPL_RETURN_ERROR_UNEXPECTED = 2
}DUPL_RETURN;

typedef struct _FRAME_DATA
{
	ID3D11Texture2D* Frame;
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;
	_Field_size_bytes_((MoveCount * sizeof(DXGI_OUTDUPL_MOVE_RECT)) + (DirtyCount * sizeof(RECT))) BYTE* MetaData;
	UINT DirtyCount;
	UINT MoveCount;
} FRAME_DATA;
