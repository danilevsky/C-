
#include "DuplicationManager.h"
#pragma comment (lib, "D3D11.lib")


DuplicationManager::DuplicationManager() : m_DeskDupl(nullptr),
m_AcquiredDesktopImage(nullptr),
m_MetaDataBuffer(nullptr),
m_MetaDataSize(0),
m_OutputNumber(0),
m_Device(nullptr)
{
	RtlZeroMemory(&m_OutputDesc, sizeof(m_OutputDesc));
}


DuplicationManager::~DuplicationManager()
{
	if (m_DeskDupl)
	{
		m_DeskDupl->Release();
		m_DeskDupl = nullptr;
	}

	if (m_AcquiredDesktopImage)
	{
		m_AcquiredDesktopImage->Release();
		m_AcquiredDesktopImage = nullptr;
	}

	if (m_MetaDataBuffer)
	{
		delete[] m_MetaDataBuffer;
		m_MetaDataBuffer = nullptr;
	}

	if (m_Device)
	{
		m_Device->Release();
		m_Device = nullptr;
	}
}

//
// Init duplication
//
DUPL_RETURN DuplicationManager::InitDupl(UINT Output)
{
	HRESULT hr = S_OK;
	// Driver types supported
	D3D_DRIVER_TYPE DriverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

	D3D_FEATURE_LEVEL FeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_1
	};
	UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);

	D3D_FEATURE_LEVEL FeatureLevel;

	// Create device
	for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
	{
		hr = D3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, D3D11_CREATE_DEVICE_VIDEO_SUPPORT, FeatureLevels, NumFeatureLevels,
			D3D11_SDK_VERSION, &m_Device, &FeatureLevel, &Context);

		if (SUCCEEDED(hr))
		{
			// Device creation success, no need to loop anymore
			m_Device->AddRef();
			break;
		}
	}
	if (FAILED(hr))
	{
		return DUPL_RETURN_ERROR_EXPECTED;
	}

	m_OutputNumber = Output;

	// Get DXGI device
	IDXGIDevice* DxgiDevice = nullptr;
	hr = m_Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));
	if (FAILED(hr))
	{
		return DUPL_RETURN_ERROR_EXPECTED;
	}

	// Get DXGI adapter
	IDXGIAdapter* DxgiAdapter = nullptr;
	hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
	DxgiDevice->Release();
	DxgiDevice = nullptr;
	if (FAILED(hr))
	{
		return DUPL_RETURN_ERROR_EXPECTED;
	}

	// Get output
	IDXGIOutput* DxgiOutput = nullptr;
	hr = DxgiAdapter->EnumOutputs(Output, &DxgiOutput);
	DxgiAdapter->Release();
	DxgiAdapter = nullptr;
	if (FAILED(hr))
	{
		return DUPL_RETURN_ERROR_EXPECTED;
	}

	DxgiOutput->GetDesc(&m_OutputDesc);

	// QI for Output 1
	IDXGIOutput1* DxgiOutput1 = nullptr;
	hr = DxgiOutput->QueryInterface(__uuidof(DxgiOutput1), reinterpret_cast<void**>(&DxgiOutput1));
	DxgiOutput->Release();
	DxgiOutput = nullptr;
	if (FAILED(hr))
	{
		return DUPL_RETURN_ERROR_EXPECTED;
	}

	// Create desktop duplication
	hr = DxgiOutput1->DuplicateOutput(m_Device, &m_DeskDupl);
	DxgiOutput1->Release();
	DxgiOutput1 = nullptr;
	if (FAILED(hr))
	{
		if (hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE)
		{
			MessageBoxW(nullptr, L"There is already the maximum number of applications using the Desktop Duplication API running, please close one of those applications and then try again.", L"Error", MB_OK);
			return DUPL_RETURN_ERROR_UNEXPECTED;
		}
		return DUPL_RETURN_ERROR_EXPECTED;
	}

	return DUPL_RETURN_SUCCESS;
}

//
// get next frame
//
_Success_(*Timeout == false && return == DUPL_RETURN_SUCCESS)
HRESULT DuplicationManager::GetFrame(_Out_ FRAME_DATA* Data)
{
	if (m_DeskDupl == nullptr) return S_FALSE;
	IDXGIResource* DesktopResource = nullptr;
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;

	// Get new frame
	HRESULT hr = m_DeskDupl->AcquireNextFrame(500, &FrameInfo, &DesktopResource);
	if (hr == DXGI_ERROR_WAIT_TIMEOUT)
	{
		return S_FALSE;
	}

	// If still holding old frame, destroy it
	if (m_AcquiredDesktopImage)
	{
		m_AcquiredDesktopImage->Release();
		m_AcquiredDesktopImage = nullptr;
	}

	// QI for IDXGIResource
	hr = DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&m_AcquiredDesktopImage));
	DesktopResource->Release();
	DesktopResource = nullptr;
	if (FAILED(hr))
	{
		return S_FALSE;
	}

	Data->Frame = m_AcquiredDesktopImage;
	Data->FrameInfo = FrameInfo;

	DoneWithFrame();

	return S_OK;
}

//
// Release frame
//
DUPL_RETURN DuplicationManager::DoneWithFrame()
{
	HRESULT hr = m_DeskDupl->ReleaseFrame();
	if (FAILED(hr))
	{
		return DUPL_RETURN_ERROR_EXPECTED;
	}

	return DUPL_RETURN_SUCCESS;
}

//
// get D3DDevice ptr
//
ID3D11Device* DuplicationManager::GetID3D11Device()
{
	return m_Device;
}
