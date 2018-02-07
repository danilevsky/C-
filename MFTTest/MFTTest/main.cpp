#include "DuplicationManager.h"
#include "H264Encode.h"

int main(int argc, char** argv)
{
	DuplicationManager* duplicationManager = new DuplicationManager();
	H264Encode* h264;
	if (duplicationManager->InitDupl(0) == DUPL_RETURN_SUCCESS)
	{
		HRESULT hr = h264->CreateInstance(duplicationManager->GetID3D11Device(), &h264);
		if (hr == S_OK)
		{
			FRAME_DATA* data = new FRAME_DATA();
			hr = duplicationManager->GetFrame(data);
			if (hr == S_OK)
			{
				hr = h264->Encode(data->Frame);
				Sleep(10000);
			}
		}
	}

	return 0;
}