
#include "H264Encode.h"


template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

template <class T> inline void SafeRelease(T*& pT)
{
	if (pT != NULL)
	{
		pT->Release();
		pT = NULL;
	}
}

H264Encode::H264Encode()
{

}

H264Encode::~H264Encode()
{
	MFShutdown();
	CoUninitialize();
}


HRESULT H264Encode::QueryInterface(REFIID riid, void** ppv)
{
	static const QITAB qit[] =
	{
		QITABENT(H264Encode, IMFAsyncCallback),
	{ 0 }
	};
	return QISearch(this, qit, riid, ppv);
}

ULONG H264Encode::AddRef()
{
	return InterlockedIncrement(&m_nRefCount);
}

ULONG H264Encode::Release()
{
	ULONG uCount = InterlockedDecrement(&m_nRefCount);
	if (uCount == 0)
		delete this;
	return uCount;
}

HRESULT H264Encode::CreateInstance(ID3D11Device * m_Device, H264Encode** ppH264)
{
	if (ppH264 == NULL)  return E_POINTER;

	H264Encode* pCore = new H264Encode();
	if (pCore == NULL)   return E_OUTOFMEMORY;

	HRESULT hr = pCore->InitEncode(m_Device);
	if (SUCCEEDED(hr))
	{
		*ppH264 = pCore;
		(*ppH264)->AddRef();
	}

	SafeRelease(&pCore);
	return hr;
}

//
// init h.264 encode
//
HRESULT H264Encode::InitEncode(ID3D11Device * m_Device)
{
	HRESULT hr2 = S_OK;
	IMFDXGIDeviceManager * ppDXVAManager;
	UINT pResetToken = 0;
	HANDLE  phDevice;
	IMFAttributes * pAttributes = NULL;
	UINT32* punValue = new UINT32();
	LPWSTR hardwareUrl = NULL;
	ID3D11VideoDevice * videoDevice;
	IDirectXVideoProcessorService* processorServer;
	IMFMediaType* outAvailableType;
	IMFMediaType* inAvailableType;
	DWORD dwTypeIndex = 0;
	GUID* inMF_MT_SUBTYPE = new GUID();
	UINT profileCount = 0;

	HRESULT hr = S_OK;
	UINT32 count = 0;

	IMFActivate **ppActivate = NULL;
	MFT_REGISTER_TYPE_INFO outType = { 0 };

	outType.guidMajorType = MFMediaType_Video;
	outType.guidSubtype = MFVideoFormat_H264;

	////Init Com
	CHECK_HR(CoInitialize(NULL), "Failed to init Com! \n");
	////Start Microsoft Media Foundation
	CHECK_HR(MFStartup(MF_VERSION), "Failed to start Microsoft Media Foundation \n");
	//Fine Hardware MFT
	hr = MFTEnumEx(MFT_CATEGORY_VIDEO_ENCODER, MFT_ENUM_FLAG_ASYNCMFT | MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_LOCALMFT | MFT_ENUM_FLAG_SORTANDFILTER | MFT_ENUM_FLAG_HARDWARE, NULL, &outType, &ppActivate, &count);
	;
	if (SUCCEEDED(hr) && count != 0)
	{
		hr = ppActivate[0]->ActivateObject(IID_PPV_ARGS(&_pTransform));
		ppActivate[0]->GetAllocatedString(MFT_ENUM_HARDWARE_URL_Attribute, &hardwareUrl, NULL);
		//hr = ppActivate[0]->GetAllocatedString(MFT_FRIENDLY_NAME_Attribute, &warename, NULL);
	}
	for (UINT32 i = 0; i < count; i++)
	{
		ppActivate[i]->Release();
	}
	CoTaskMemFree(ppActivate);

	MFCreateAttributes(&pAttributes, 10);
	hr2 = _pTransform->GetAttributes(&pAttributes);
	CHECK_HR(pAttributes->GetUINT32(MF_SA_D3D11_AWARE, punValue), "Failed to FindEncoder \n");
	if (punValue == 0) goto done;

	hr2 = pAttributes->SetUINT32(MF_TRANSFORM_ASYNC, 1);
	hr2 = pAttributes->SetString(MFT_ENUM_HARDWARE_URL_Attribute, hardwareUrl);
	hr2 = pAttributes->SetUINT32(MFT_SUPPORT_DYNAMIC_FORMAT_CHANGE, 1);
	hr2 = pAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, 1);
	hr2 = pAttributes->SetUINT32(MF_TRANSFORM_ASYNC_UNLOCK, 1);
	
	CHECK_HR(MFCreateDXGIDeviceManager(&pResetToken, &ppDXVAManager), "Failed to create MFCreateDXGIDeviceManager \n");
	CHECK_HR(ppDXVAManager->ResetDevice(m_Device, (UINT)pResetToken), "Failed to ResetDevice \n");
	CHECK_HR(ppDXVAManager->OpenDeviceHandle(&phDevice), "Failed to OpenDeviceHandle");

	hr2 = pAttributes->SetUnknown(MF_SINK_WRITER_D3D_MANAGER, ppDXVAManager);

	//Create MFTOutputMediaType
	MFCreateMediaType(&pMFTOutputMediaType);
	CHECK_HR(pMFTOutputMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video), "Failed to set media type MF_MT_MAJOR_TYPE \n");
	CHECK_HR(pMFTOutputMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264), "Failed to set media type MF_MT_SUBTYPE \n");
	CHECK_HR(pMFTOutputMediaType->SetUINT32(MF_MT_AVG_BITRATE, TARGET_AVERAGE_BIT_RATE), "Failed to set media type MF_MT_AVG_BITRATE \n");
	CHECK_HR(MFSetAttributeRatio(pMFTOutputMediaType, MF_MT_FRAME_RATE, TARGET_FRAME_RATE, 1), "Failed to set media type MF_MT_FRAME_RATE \n");
	CHECK_HR(MFSetAttributeSize(pMFTOutputMediaType, MF_MT_FRAME_SIZE, CAMERA_RESOLUTION_WIDTH, CAMERA_RESOLUTION_HEIGHT), "Failed to set media type MF_MT_FRAME_SIZE \n");
	CHECK_HR(pMFTOutputMediaType->SetUINT32(MF_MT_INTERLACE_MODE, 2), "Failed to set media type MF_MT_INTERLACE_MODE \n");																																																										//H.264±àÂëÅäÖÃÎÄ¼þ
	CHECK_HR(pMFTOutputMediaType->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_Base), "Failed to set media type MF_MT_MPEG2_PROFILE \n");
	CHECK_HR(MFSetAttributeRatio(pMFTOutputMediaType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1), "Failed to set media type MF_MT_PIXEL_ASPECT_RATIO \n");
	CHECK_HR(pMFTOutputMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE), "Failed to set media type MF_MT_ALL_SAMPLES_INDEPENDENT \n");
	CHECK_HR(pMFTOutputMediaType->SetUINT32(CODECAPI_AVEncCommonRateControlMode, eAVEncCommonRateControlMode_GlobalLowDelayVBR), "Failed to set media type CODECAPI_AVEncCommonRateControlMode");
	CHECK_HR(pMFTOutputMediaType->SetUINT32(CODECAPI_AVEncCommonQuality, 70), "Failed to set media type CODECAPI_AVEncCommonQuality \n");
	CHECK_HR(pMFTOutputMediaType->SetUINT32(CODECAPI_AVEncAdaptiveMode, eAVEncAdaptiveMode_None), "Failed to set CODECAPI_AVEncAdaptiveMode \n");
	CHECK_HR(pMFTOutputMediaType->SetUINT32(CODECAPI_AVEncCommonBufferSize, AVEncCommonBufferSize), "Failed to set CODECAPI_AVEncCommonBufferSize \n");
	CHECK_HR(pMFTOutputMediaType->SetUINT32(CODECAPI_AVEncCommonMeanBitRate, TARGET_AVERAGE_BIT_RATE), "Failed to set CODECAPI_AVEncCommonMeanBitRate \n");
	CHECK_HR(pMFTOutputMediaType->SetUINT32(CODECAPI_AVEncCommonQualityVsSpeed, 50), "Failed to CODECAPI_AVEncCommonQualityVsSpeed \n");
	CHECK_HR(pMFTOutputMediaType->SetUINT32(CODECAPI_AVEncH264SPSID, 31), "Failed to set CODECAPI_AVEncCommonMaxBitRate \n");
	CHECK_HR(pMFTOutputMediaType->SetUINT32(CODECAPI_AVEncMPVDefaultBPictureCount, 0), "Failed to set CODECAPI_AVEncCommonMaxBitRate \n");
	CHECK_HR(pMFTOutputMediaType->SetUINT32(CODECAPI_AVEncCommonMaxBitRate, TARGET_AVERAGE_BIT_RATE), "Failed to set CODECAPI_AVEncCommonMaxBitRate \n");
	CHECK_HR(pMFTOutputMediaType->SetUINT32(CODECAPI_AVEncMPVGOPSize, 1), "Failed to set CODECAPI_AVEncMPVGOPSize \n");
	CHECK_HR(pMFTOutputMediaType->SetUINT32(CODECAPI_AVEncVideoEncodeQP, 24), "Failed to set CODECAPI_AVEncVideoEncodeQP \n");
	CHECK_HR(pMFTOutputMediaType->SetUINT32(CODECAPI_AVEncVideoForceKeyFrame, 1), "Failed to set CODECAPI_AVEncVideoForceKeyFrame \n");
	CHECK_HR(pMFTOutputMediaType->SetUINT32(CODECAPI_AVLowLatencyMode, 1), "Failed to set CODECAPI_AVEncVideoForceKeyFrame \n");
	CHECK_HR(pMFTOutputMediaType->SetUINT32(CODECAPI_AVEncVideoMinQP, 1), "Failed to set CODECAPI_AVEncVideoMinQP \n");
	CHECK_HR(pMFTOutputMediaType->SetUINT32(CODECAPI_AVEncVideoLTRBufferControl, 0), "Failed to set CODECAPI_AVEncVideoLTRBufferControl \n");

	CHECK_HR(_pTransform->SetOutputType(0, pMFTOutputMediaType, 0), "Failed to set media type SetOutputType \n");

	//Create MFTInputMediaType
	MFCreateMediaType(&pMFTInputMediaType);
	CHECK_HR(pMFTInputMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video), "Failed to set inputmediatype MF_MT_MAJOR_TYPE \n");
	CHECK_HR(pMFTInputMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12), "Failed to set inputmediatype MF_MT_SUBTYPE \n");
	CHECK_HR(MFSetAttributeSize(pMFTInputMediaType, MF_MT_FRAME_SIZE, CAMERA_RESOLUTION_WIDTH, CAMERA_RESOLUTION_HEIGHT), "Failed to set inputmediatype MF_MT_FRAME_SIZE");
	CHECK_HR(MFSetAttributeRatio(pMFTInputMediaType, MF_MT_FRAME_RATE, TARGET_FRAME_RATE, 1), "Failed to set inputmediatype MF_MT_FRAME_RATE");
	CHECK_HR(MFSetAttributeRatio(pMFTInputMediaType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1), "Failed to set inputmediatype MF_MT_PIXEL_ASPECT_RATIO");
	CHECK_HR(pMFTInputMediaType->SetUINT32(MF_MT_INTERLACE_MODE, 2), "Failed to set inputmediatype MF_MT_INTERLACE_MODE");

	CHECK_HR(_pTransform->SetInputType(0, pMFTInputMediaType, 0), "Failed to set inputmediatype");
	CHECK_HR(_pTransform->GetInputStatus(0, &mftStatus), "Failed to GetInputStatus \n");
	if (MFT_INPUT_STATUS_ACCEPT_DATA != mftStatus) {
	    goto done;
	}

	//Get MFMediaEventGenerator
	hr2 = _pTransform->QueryInterface(IID_IMFMediaEventGenerator, (void**)&mpH264EncoderEventGenerator);
	//Send D3D Message
	hr2 = _pTransform->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)ppDXVAManager);
	//Begin Event
	hr2 = mpH264EncoderEventGenerator->BeginGetEvent(this, NULL);

	memset(&_outputDataBuffer, 0, sizeof _outputDataBuffer);

	return S_OK;
done:
	return S_FALSE;
}

HRESULT H264Encode::Invoke(IMFAsyncResult* pAsyncResult)
{
	HRESULT hr = S_OK;
	HRESULT hStatus;
	IMFMediaEvent* pEvent;
	MediaEventType meType;
	IMFMediaBuffer *pBuffer = NULL;
	IMFSample *mftOutSample = NULL;
	DWORD processOutputStatus = 0;
	MFT_OUTPUT_STREAM_INFO StreamInfo;

	hr = mpH264EncoderEventGenerator->EndGetEvent(pAsyncResult, &pEvent);
	if (FAILED(hr))
		return hr;

	hr = pEvent->GetType(&meType);
	hr = pEvent->GetStatus(&hStatus);
	pEvent->Release();
	if (FAILED(hr))
		return hr;

	if (hStatus == S_OK)
	{
		//Input
		if (meType == METransformNeedInput)
		{
			IMFSample *videoSample = inputIMFSample;

			hr = _pTransform->ProcessInput(0, videoSample, 0);
		}
		else if (meType == METransformHaveOutput)
		{
			_pTransform->GetOutputStreamInfo(0, &StreamInfo);

			MFCreateSample(&mftOutSample);
			MFCreateMemoryBuffer(StreamInfo.cbSize, &pBuffer);
			mftOutSample->AddBuffer(pBuffer);

			while (true)
			{
				_outputDataBuffer.dwStreamID = 0;
				_outputDataBuffer.dwStatus = 0;
				_outputDataBuffer.pEvents = NULL;
				_outputDataBuffer.pSample = mftOutSample;

				hr = _pTransform->ProcessOutput(0, 1, &_outputDataBuffer, &processOutputStatus);

				if (hr != MF_E_TRANSFORM_NEED_MORE_INPUT)
				{
					_outputDataBuffer.pSample->SetSampleTime(rtStart);
					_outputDataBuffer.pSample->SetSampleDuration(0);

					IMFMediaBuffer *buf = NULL;
					BYTE* ToBuffer = NULL;
					DWORD bufLength;
					_outputDataBuffer.pSample->ConvertToContiguousBuffer(&buf);
					buf->GetCurrentLength(&bufLength);
					BYTE * rawBuffer = NULL;

					auto now = GetTickCount();

					buf->Lock(&rawBuffer, NULL, NULL);
					memmove(ToBuffer, rawBuffer, bufLength);
					buf->Unlock();

					SafeRelease(&buf);
				}

				SafeRelease(&pBuffer);
				SafeRelease(&mftOutSample);

				break;
			}
		}
	}

	return hr;
}

HRESULT H264Encode::Encode(ID3D11Texture2D* m_AcquiredDesktopImage)
{
	DWORD processOutputStatus = 0;
	IMFSample *videoSample = NULL;
	DWORD streamIndex, flags;
	HRESULT mftProcessInput = S_OK;
	HRESULT mftProcessOutput = S_OK;
	MFT_OUTPUT_STREAM_INFO StreamInfo;
	IMFMediaBuffer *pBuffer = NULL;
	IMFSample *mftOutSample = NULL;
	DWORD mftOutFlags;
	bool frameSent = false;
	IMFMediaBuffer *iBuffer = NULL;
	LONGLONG llVideoTimeStamp = 0, llSampleDuration = 0;

	CHECK_HR(MFCreateDXGISurfaceBuffer(__uuidof(ID3D11Texture2D), m_AcquiredDesktopImage, 0, FALSE, &iBuffer), "Error reading video sample.\n");
	CHECK_HR(MFCreateVideoSampleFromSurface(NULL, &videoSample), "Fail to create VideosampleSurface! \n");
	CHECK_HR(videoSample->AddBuffer(iBuffer), "Fail to addbuffer! \n");

	if (videoSample) {

		CHECK_HR(videoSample->SetSampleTime(rtStart), "Error setting the video sample time.\n");
		CHECK_HR(videoSample->SetSampleDuration(VIDEO_FRAME_DURATION), "Error setting video sample duration.\n");

		inputIMFSample = videoSample;
		
		if (rtStart == 0)
			CHECK_HR(_pTransform->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, NULL), "Failed to process START_OF_STREAM command on H.264 MFT.\n");
	}

	rtStart += VIDEO_FRAME_DURATION;
	return S_OK;
done:
	return S_FALSE;
}