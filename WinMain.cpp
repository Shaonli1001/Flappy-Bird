#include <Windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <iostream>
#include <time.h>
#include <random>
#include <sstream>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

#define DEGTORAD (D3DX_PI/(float)180)

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
int AppCode(HINSTANCE, int);
bool CheckOverlap(D3DXVECTOR3, D3DXVECTOR3);
void StopGame();
float RandomizeLocX();
bool CalculateScore(D3DXVECTOR3 playerLoc, D3DXVECTOR3 pillarLoc);

const int width = 1280;
const int height = 720;
LPCWSTR WindowClassName = L"Flappy Bird";
HWND hWnd;

IDirect3D9* pD3D9;
IDirect3DDevice9* pD3DDevice9;
ID3DXFont* pFont = nullptr;
ID3DXSprite* pSprite = nullptr;

const float fovDegrees = 60.0f;
D3DXMATRIX transformMatrix;
D3DXMATRIX pillarTransformMatrix;

D3DXVECTOR3 vPlayerLoc(-1.2f, 0.5f, 0);
D3DXVECTOR3 vLowPillarLoc(-0.5f, -1.0f, 0);
D3DXVECTOR3 vLowPillarLoc2(2.0f, -1.0f, 0);
D3DXVECTOR3 vTopPillarLoc(0.7f, 1.0f, 0);
D3DXVECTOR3 vTopPillarLoc2(2.5f, 1.0f, 0);
D3DXVECTOR3 vInertia(0, 0, 0);

D3DXVECTOR3 vPillarSize(0.2f, 0.5f, 0.2f);
D3DXVECTOR3 vPlayerSize(0.2f, 0.2f, 0.2f);

D3DLIGHT9 light;
D3DMATERIAL9 material;

float gravity = 0.00005f;
float delShiftY = 0.0005f;

int score = 0;

bool bIsOverlapping = false;
bool bPauseGame = false;

#define SCORE_WSTRING_LENGTH 64
WCHAR wCharArrayString[SCORE_WSTRING_LENGTH] = L"Score: 000000";
WCHAR* pScoreString = wCharArrayString;
LPWSTR lpwstrScore = wCharArrayString;

IDirect3DVertexBuffer9* pPlayerVertexBuffer;
IDirect3DVertexBuffer9* pPillarVertexBuffer;
IDirect3DIndexBuffer9* pPlayerIndexBuffer;

const DWORD VertexFVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE; 

struct Vertex
{
	float x, y, z;
	float nx, ny, nz;
	D3DCOLOR color;
};

int main()
{
	printf("Application Debug Mode\n");
	AppCode(GetModuleHandle(nullptr), SW_SHOW);
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	AppCode(hInstance, nCmdShow);
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		if (wParam == 32)
		{
			vInertia.y = delShiftY;
		}
		if (wParam == 27)
		{
			bPauseGame = !bPauseGame;
		}
		break;
	case WM_KEYUP:
		if (wParam == 32)
		{
			vInertia.y = 0;
		}
		break;
	default:
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int AppCode(HINSTANCE hInstance, int nCmdShow)
{
	WNDCLASSEX wcx;
	wcx.cbSize = sizeof(WNDCLASSEX);
	wcx.hInstance = hInstance;
	wcx.lpfnWndProc = WindowProc;
	wcx.lpszClassName = WindowClassName;

	wcx.style = CS_HREDRAW | CS_VREDRAW;

	wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcx.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wcx.hbrBackground = (HBRUSH)(COLOR_BACKGROUND);

	wcx.lpszMenuName = nullptr;
	wcx.cbClsExtra = NULL;
	wcx.cbWndExtra = NULL;

	RegisterClassEx(&wcx);

	hWnd = CreateWindowEx(
		NULL,
		WindowClassName,
		L"Flappy Bird",
		WS_OVERLAPPEDWINDOW,
		10,
		10,
		width,
		height,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	pD3D9 = Direct3DCreate9(D3D_SDK_VERSION);
	if (!pD3D9)
	{
		return -1;
	}

	D3DCAPS9 deviceCaps;
	pD3D9->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &deviceCaps);

	int vertexProcessing = 0;
	if (deviceCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
	{
		vertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	}
	else
	{
		vertexProcessing = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	}

	D3DPRESENT_PARAMETERS d3dpp;

	d3dpp.BackBufferWidth = width;
	d3dpp.BackBufferHeight = height;
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dpp.BackBufferCount = 1;

	d3dpp.hDeviceWindow = hWnd;
	d3dpp.Windowed = true;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.MultiSampleQuality = NULL;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;

	d3dpp.Flags = NULL;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

	pD3D9->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		vertexProcessing,
		&d3dpp,
		&pD3DDevice9
	);

	pD3D9->Release();

	HRESULT hr;
	hr = D3DXCreateSprite(pD3DDevice9, &pSprite);
	int fontSize = 48;

	hr = D3DXCreateFont(
		pD3DDevice9,
		fontSize,
		0,
		FW_NORMAL,
		1,
		false,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		ANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		L"Arial",
		&pFont
	);

	RECT rectText;
	rectText.left = 20;
	rectText.top = 20;
	rectText.right = rectText.left + 200;
	rectText.bottom = rectText.top + fontSize;

	pD3DDevice9->CreateVertexBuffer(
		8 * sizeof(Vertex),
		D3DUSAGE_WRITEONLY,
		VertexFVF,
		D3DPOOL_MANAGED,
		&pPlayerVertexBuffer,
		nullptr
	);

	Vertex* pPlayerVertices;
	pPlayerVertexBuffer->Lock(0, 0, (void**)&pPlayerVertices, 0);

	pPlayerVertices[0].x = -vPlayerSize.x;
	pPlayerVertices[0].y = vPlayerSize.y;
	pPlayerVertices[0].z = vPlayerSize.z;
	pPlayerVertices[0].nx = -1.0f;
	pPlayerVertices[0].ny = 1.0f;
	pPlayerVertices[0].nz = -1.0f;
	pPlayerVertices[0].color = 0xffff0000;

	pPlayerVertices[1].x = vPlayerSize.x;
	pPlayerVertices[1].y = vPlayerSize.y;
	pPlayerVertices[1].z = vPlayerSize.z;
	pPlayerVertices[1].nx = 1.0f;
	pPlayerVertices[1].ny = 1.0f;
	pPlayerVertices[1].nz = -1.0f;
	pPlayerVertices[1].color = 0xffff0000;

	pPlayerVertices[2].x = -vPlayerSize.x;
	pPlayerVertices[2].y = -vPlayerSize.y;
	pPlayerVertices[2].z = vPlayerSize.z;
	pPlayerVertices[2].nx = -1.0f;
	pPlayerVertices[2].ny = -1.0f;
	pPlayerVertices[2].nz = -1.0f;
	pPlayerVertices[2].color = 0xffff0000;

	pPlayerVertices[3].x = vPlayerSize.x;
	pPlayerVertices[3].y = -vPlayerSize.y;
	pPlayerVertices[3].z = vPlayerSize.z;
	pPlayerVertices[3].nx = 1.0f;
	pPlayerVertices[3].ny = -1.0f;
	pPlayerVertices[3].nz = -1.0f;
	pPlayerVertices[3].color = 0xffff0000;

	pPlayerVertices[4].x = -vPlayerSize.x;
	pPlayerVertices[4].y = vPlayerSize.y;
	pPlayerVertices[4].z = vPlayerSize.z;
	pPlayerVertices[4].nx = -1.0f;
	pPlayerVertices[4].ny = 1.0f;
	pPlayerVertices[4].nz = 1.0f;
	pPlayerVertices[4].color = 0xffff0000;

	pPlayerVertices[5].x = vPlayerSize.x;
	pPlayerVertices[5].y = vPlayerSize.y;
	pPlayerVertices[5].z = vPlayerSize.z;
	pPlayerVertices[5].nx = 1.0f;
	pPlayerVertices[5].ny = 1.0f;
	pPlayerVertices[5].nz = 1.0f;
	pPlayerVertices[5].color = 0xffff0000;

	pPlayerVertices[6].x = -vPlayerSize.x;
	pPlayerVertices[6].y = -vPlayerSize.y;
	pPlayerVertices[6].z = vPlayerSize.z;
	pPlayerVertices[6].nx = -1.0f;
	pPlayerVertices[6].ny = -1.0f;
	pPlayerVertices[6].nz = 1.0f;
	pPlayerVertices[6].color = 0xffff0000;

	pPlayerVertices[7].x = vPlayerSize.x;
	pPlayerVertices[7].y = -vPlayerSize.y;
	pPlayerVertices[7].z = vPlayerSize.z;
	pPlayerVertices[7].nx = 1.0f;
	pPlayerVertices[7].ny = -1.0f;
	pPlayerVertices[7].nz = 1.0f;
	pPlayerVertices[7].color = 0xffff0000;

	pPlayerVertexBuffer->Unlock();

	pD3DDevice9->CreateVertexBuffer(
		8 * sizeof(Vertex),
		D3DUSAGE_WRITEONLY,
		VertexFVF,
		D3DPOOL_MANAGED,
		&pPillarVertexBuffer,
		nullptr
	);

	Vertex* pPillarVertices;
	pPillarVertexBuffer->Lock(0, 0, (void**)&pPillarVertices, 0);

	pPillarVertices[0] = { -vPillarSize.x,  vPillarSize.y,  vPillarSize.z, -1.0f,  1.0f, -1.0f, 0xff00ff00 };
	pPillarVertices[1] = {  vPillarSize.x,  vPillarSize.y,  vPillarSize.z,  1.0f,  1.0f, -1.0f, 0xff00ff00 };
	pPillarVertices[2] = { -vPillarSize.x, -vPillarSize.y,  vPillarSize.z, -1.0f, -1.0f, -1.0f, 0xff00ff00 };
	pPillarVertices[3] = {  vPillarSize.x, -vPillarSize.y,  vPillarSize.z,  1.0f, -1.0f, -1.0f, 0xff00ff00 };
	pPillarVertices[4] = { -vPillarSize.x,  vPillarSize.y, -vPillarSize.z, -1.0f,  1.0f,  1.0f, 0xff00ff00 };
	pPillarVertices[5] = {  vPillarSize.x,  vPillarSize.y, -vPillarSize.z,  1.0f,  1.0f,  1.0f, 0xff00ff00 };
	pPillarVertices[6] = { -vPillarSize.x, -vPillarSize.y, -vPillarSize.z, -1.0f, -1.0f,  1.0f, 0xff00ff00 };
	pPillarVertices[7] = {  vPillarSize.x, -vPillarSize.y, -vPillarSize.z,  1.0f, -1.0f,  1.0f, 0xff00ff00 };

	pPlayerVertexBuffer->Unlock();


	pD3DDevice9->CreateIndexBuffer(
		36 * sizeof(WORD),
		D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16,
		D3DPOOL_MANAGED,
		&pPlayerIndexBuffer,
		nullptr
	);

	WORD* pPlayerIndices;
	pPlayerIndexBuffer->Lock(0, 0, (void**)&pPlayerIndices, 0);

	pPlayerIndices[0] = 0;
	pPlayerIndices[1] = 1;
	pPlayerIndices[2] = 2;
	pPlayerIndices[3] = 2;
	pPlayerIndices[4] = 1;
	pPlayerIndices[5] = 3;
	pPlayerIndices[6] = 4;
	pPlayerIndices[7] = 5;
	pPlayerIndices[8] = 6;
	pPlayerIndices[9] = 5;
	pPlayerIndices[10] = 7;
	pPlayerIndices[11] = 6;
	pPlayerIndices[12] = 4;
	pPlayerIndices[13] = 0;
	pPlayerIndices[14] = 6;
	pPlayerIndices[15] = 6;
	pPlayerIndices[16] = 0;
	pPlayerIndices[17] = 2;
	pPlayerIndices[18] = 6;
	pPlayerIndices[19] = 7;
	pPlayerIndices[20] = 2;
	pPlayerIndices[21] = 2;
	pPlayerIndices[22] = 7;
	pPlayerIndices[23] = 3;
	pPlayerIndices[24] = 1;
	pPlayerIndices[25] = 3;
	pPlayerIndices[26] = 7;
	pPlayerIndices[27] = 5;
	pPlayerIndices[28] = 1;
	pPlayerIndices[29] = 7;
	pPlayerIndices[30] = 4;
	pPlayerIndices[31] = 5;
	pPlayerIndices[32] = 1;
	pPlayerIndices[33] = 4;
	pPlayerIndices[34] = 1;
	pPlayerIndices[35] = 0;

	pPlayerIndexBuffer->Unlock();

	ZeroMemory(&light, sizeof(D3DLIGHT9));
	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	light.Direction = D3DXVECTOR3(0, 0, -1.0f);
	light.Position = D3DXVECTOR3(0, 0, -2.0f);
	light.Range = 5.0f;

	ZeroMemory(&material, sizeof(D3DMATERIAL9));
	material.Diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	material.Ambient = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);

	pD3DDevice9->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	pD3DDevice9->SetRenderState(D3DRS_LIGHTING, true);
	pD3DDevice9->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_PHONG);
	pD3DDevice9->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	pD3DDevice9->SetRenderState(D3DRS_ZENABLE, true);
	pD3DDevice9->SetRenderState(D3DRS_NORMALIZENORMALS, true);

	pD3DDevice9->SetLight(0, &light);
	pD3DDevice9->LightEnable(0, true);

	D3DXVECTOR3 camPos(0, 0, -2.0f);
	D3DXVECTOR3 camLookAt(0, 0, 0);
	D3DXVECTOR3	camUpDir(0, 1.0f, 0);
	D3DXMATRIX viewMatrix;

	D3DXMatrixLookAtLH(
		&viewMatrix,
		&camPos,
		&camLookAt,
		&camUpDir
	);

	pD3DDevice9->SetTransform(D3DTS_VIEW, &viewMatrix);

	D3DXMATRIX projectionMatrix;

	D3DXMatrixPerspectiveFovLH(
		&projectionMatrix,
		DEGTORAD * fovDegrees,
		(float)width / (float)height,
		0.001f,
		1000.0f
	);

	pD3DDevice9->SetTransform(D3DTS_PROJECTION, &projectionMatrix);

	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			if (CalculateScore(vPlayerLoc, vLowPillarLoc) || CalculateScore(vPlayerLoc, vLowPillarLoc2) || CalculateScore(vPlayerLoc, vTopPillarLoc) || CalculateScore(vPlayerLoc, vTopPillarLoc2))
			{
				score += 1;
			}
			if (!bPauseGame)
			{
				vPlayerLoc.y -= gravity;
				vPlayerLoc += vInertia;
				vLowPillarLoc.x -= 0.00005f;
				vTopPillarLoc.x -= 0.00005f;
				vLowPillarLoc2.x -= 0.00005f;
				vTopPillarLoc2.x -= 0.00005f;
				
				wsprintf(lpwstrScore, L"Score: %d", score);
			}

			if (vLowPillarLoc.x < -2.2f)
			{
				vLowPillarLoc.x = RandomizeLocX();
			}

			if (vTopPillarLoc.x < -2.2f)
			{
				vTopPillarLoc.x = RandomizeLocX();
			}

			if (vLowPillarLoc2.x < -2.2f)
			{
				vLowPillarLoc2.x = RandomizeLocX();
			}

			if (vTopPillarLoc2.x < -2.2f)
			{
				vTopPillarLoc2.x = RandomizeLocX();
			}

			if (vPlayerLoc.y < -2.0f || vPlayerLoc.y > 2.0f)
			{
				StopGame();
			}
			
			if (CheckOverlap(vPlayerLoc, vLowPillarLoc) || CheckOverlap(vPlayerLoc, vTopPillarLoc) ||  CheckOverlap(vPlayerLoc, vLowPillarLoc2) || CheckOverlap(vPlayerLoc, vTopPillarLoc2))
			{
				StopGame();
			}

			if (pD3DDevice9)
			{
				pD3DDevice9->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff61a6c2, 1.0f, 0);
				pD3DDevice9->BeginScene();

				pD3DDevice9->SetStreamSource(0, pPlayerVertexBuffer, 0, sizeof(Vertex));
				pD3DDevice9->SetFVF(VertexFVF);
				pD3DDevice9->SetIndices(pPlayerIndexBuffer);
				pD3DDevice9->SetMaterial(&material);

				if (pFont)
				{
					pSprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE);
					pFont->DrawText(
						pSprite,
						lpwstrScore,
						-1,
						&rectText,
						DT_NOCLIP,
						0xffff0000
					);
					pSprite->End();
				}

				// PLAYER
				D3DXMatrixTranslation(&transformMatrix, vPlayerLoc.x, vPlayerLoc.y, vPlayerLoc.z);
				pD3DDevice9->SetTransform(D3DTS_WORLD, &transformMatrix);
				pD3DDevice9->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 12);

				// LOWER PILLAR
				pD3DDevice9->SetStreamSource(0, pPillarVertexBuffer, 0, sizeof(Vertex));
				D3DXMatrixTranslation(&pillarTransformMatrix, vLowPillarLoc.x, vLowPillarLoc.y, vLowPillarLoc.z);
				pD3DDevice9->SetTransform(D3DTS_WORLD, &pillarTransformMatrix);
				pD3DDevice9->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 12);

				// UPPER PILLAR
				D3DXMatrixIdentity(&pillarTransformMatrix);
				D3DXMatrixTranslation(&pillarTransformMatrix, vTopPillarLoc.x, vTopPillarLoc.y, vTopPillarLoc.z);
				pD3DDevice9->SetTransform(D3DTS_WORLD, &pillarTransformMatrix);
				pD3DDevice9->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 12);

				// SECOND LOWER PILLAR
				D3DXMatrixIdentity(&pillarTransformMatrix);
				D3DXMatrixTranslation(&pillarTransformMatrix, vLowPillarLoc2.x, vLowPillarLoc2.y, vLowPillarLoc2.z);
				pD3DDevice9->SetTransform(D3DTS_WORLD, &pillarTransformMatrix);
				pD3DDevice9->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 12);

				// SECOND UPPER PILLAR
				D3DXMatrixIdentity(&pillarTransformMatrix);
				D3DXMatrixTranslation(&pillarTransformMatrix, vTopPillarLoc2.x, vTopPillarLoc2.y, vTopPillarLoc2.z);
				pD3DDevice9->SetTransform(D3DTS_WORLD, &pillarTransformMatrix);
				pD3DDevice9->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 12);

				pD3DDevice9->EndScene();
				pD3DDevice9->Present(0, 0, 0, 0);
			}
		}
	}
	return 0;
}

bool CheckOverlap(D3DXVECTOR3 playerLoc, D3DXVECTOR3 pillarLoc)
{
	bool bOverlapH = false;
	bool bOverlapV = false;


	// HORIZONTAL OVERLAP 
	if (playerLoc.x < pillarLoc.x)
	{
		// LEFT
		if ((playerLoc.x + vPlayerSize.x) > (pillarLoc.x - vPillarSize.x))
		{
			bOverlapH = true;
		}
	}
	else if (playerLoc.x > pillarLoc.x)
	{
		// RIGHT
		if ((playerLoc.x - vPlayerSize.x) < (pillarLoc.x + vPillarSize.x))
		{
			bOverlapH = true;
		}
	}
	else if (playerLoc.x == pillarLoc.x)
	{
		bOverlapH = true;
	}

	// VERTICAL OVERLAP 
	if (playerLoc.y > pillarLoc.y)
	{
		// BOTTOM
		if ((playerLoc.y - vPlayerSize.y) < (pillarLoc.y + vPillarSize.y))
		{
			bOverlapV = true;
		}
	}
	else if (playerLoc.y < pillarLoc.y)
	{
		// TOP
		if ((playerLoc.y + vPlayerSize.y) > (pillarLoc.y - vPillarSize.y))
		{
			bOverlapV = true;
		}
	}
	else if (playerLoc.y == pillarLoc.y)
	{
		bOverlapV = true;
	}

	bIsOverlapping = bOverlapH && bOverlapV;

	return bIsOverlapping;
}

void StopGame()
{

	vPlayerLoc = { -1.2f, 0.5f, 0 } ;
	vLowPillarLoc = { -0.5f, -1.0f, 0 };
	vTopPillarLoc = { 0.7f, 1.0f, 0 };
	vLowPillarLoc2 = { 2.0f, -1.0f, 0 };
	vTopPillarLoc2 = { 2.5f, 1.0f, 0 };
	score = 0;
}

float RandomizeLocX()
{
	std::random_device rd;  // Seed
	std::mt19937 gen(rd()); // Mersenne Twister PRNG

	std::uniform_real_distribution<> dis(2.0f, 4.0f);

	double x = dis(gen);
	return (float)x;
}

bool CalculateScore(D3DXVECTOR3 playerLoc, D3DXVECTOR3 pillarLoc)
{
	bool crossedPlayer = false;
	if (pillarLoc.x <= playerLoc.x && pillarLoc.x > playerLoc.x - 0.00005)
	{
		crossedPlayer = true;
	}
	return crossedPlayer;
}
