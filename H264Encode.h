#pragma once
#include <d3d11.h>
#include <d3d9types.h>
#include <mfapi.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <wmcodecdsp.h>
#include <Codecapi.h>
#include <Mftransform.h>
#include <initguid.h>
#include <mfobjects.h>
#include "CommonTypes.h"
#include<dxva2api.h>
#include <shlwapi.h>
#include<queue>

#pragma comment(lib, "d3d11.lib")   
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfplay.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")
#pragma comment(lib, "Evr.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace std;
//
// H264 Encode Class
//
class H264Encode :public IMFAsyncCallback
{
public:
	H264Encode();
	virtual ~H264Encode(void);
	static HRESULT CreateInstance(ID3D11Device * m_Device, H264Encode** ppH264);

	// IUnknown  
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	HRESULT InitEncode(ID3D11Device * m_Device);
	HRESULT Encode(ID3D11Texture2D* m_AcquiredDesktopImage);
	STDMETHODIMP GetParameters(DWORD*, DWORD*)
	{
		return E_NOTIMPL;
	}
	STDMETHODIMP  Invoke(IMFAsyncResult* pAsyncResult);


protected:
	long  m_nRefCount;

	IMFTransform*  _pTransform; //H.264 Encode MFT
	IUnknown *spTransformUnk = NULL;
	IMFMediaType *pMFTInputMediaType = NULL, *pMFTOutputMediaType = NULL;
	MFT_OUTPUT_DATA_BUFFER _outputDataBuffer;
	IMFMediaEventGenerator * mpH264EncoderEventGenerator;

	static const int TARGET_AVERAGE_BIT_RATE = 10000000;  
	static const int AVEncCommonBufferSize = 63323000;
	static const int CAMERA_RESOLUTION_WIDTH = 1920;
	static const int CAMERA_RESOLUTION_HEIGHT = 1080;
	static const int DEVICE_INDEX = 0;
	static const int TARGET_FRAME_RATE = 30;
	LONGLONG VIDEO_FRAME_DURATION = 10 * 1000 * 1000 / TARGET_FRAME_RATE;
	LONGLONG rtStart = 0; 

	DWORD mftStatus = 0;
	int frameCount = 0;

	IMFSample* inputIMFSample;
};
