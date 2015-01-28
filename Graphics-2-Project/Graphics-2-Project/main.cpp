/**************************************************************/
// Graphics 2 Project 1501
// Author: Jordan Ness
// File: main.cpp
// purpose: the starting point for the application


/*************************************************************/
// Includes and #defines


#include <iostream>
#include <ctime>
#include "XTime.h"
#include <vector>
#include <fbxsdk.h>
#include "DDSTextureLoader.h"
#include <mutex>
#include <condition_variable>
#include <thread>

//Add xtimeclass here

using namespace std;
//Include the direct x api and lib
#include <d3d11.h>
#pragma comment(lib, "D3D11.lib")
#include <d3d11sdklayers.h>
//Include the direct x math library
#include <DirectXMath.h>
using namespace DirectX;

//include shaders
#include "Trivial_PS.csh"
#include "Trivial_VS.csh"
#include "Skybox_VS.csh"
#include "Skybox_PS.csh"


//Create the backbuffer size
#define BACKBUFFER_WIDTH 1280
#define BACKBUFFER_HEIGHT 720

class APPLICATION
{
	struct SIMPLE_VERTEX;
	//Used by windows code
	HINSTANCE							application;
	WNDPROC								appWndProc;
	HWND								window;

	//Create DirectX device variables
	ID3D11Device* pDevice; 
	ID3D11DeviceContext* pDeviceContext;
	ID3D11DeviceContext* pDeferredContext = nullptr;
	ID3D11DeviceContext* pGeoDeferredContext = nullptr;
	ID3D11RenderTargetView* pRTV;
	IDXGISwapChain* pSwapChain = nullptr;
	ID3D11Resource* pBackBuffer;
	D3D11_VIEWPORT vp;
	ID3D11ShaderResourceView* pSRView;
	ID3D11SamplerState* pSamplerState;
	ID3D11ShaderResourceView* pCharacterSRV;
	ID3D11ShaderResourceView* NullSRV = nullptr;

	//Create Buffer variables

	//Star Vertex Buffer info
	//ID3D11Buffer* pVertexBuffer; //The buffer of vertices to be drawn
	ID3D11Buffer* pStarVertBuffer = nullptr;
	unsigned int TriVerts = 60;  //The number of vertices we will need
	ID3D11InputLayout* pInputLayout; //The input layout



	//Skybox vertex buffer info
	ID3D11Buffer* pSBVertexBuffer;
	int SBNumVerts;
	unsigned int SBNumFaces;
	vector<SIMPLE_VERTEX> SkyBoxVerts;

	//Character Model Vertex buffer 
	ID3D11Buffer* pCharacterVertexBuffer;
	vector<SIMPLE_VERTEX> CharacterVerts;

	//Constant Buffers
	ID3D11Buffer* pObjectCBuffer;
	ID3D11Buffer* pSceneCBuffer;
	ID3D11Buffer* pIndexBuffer;
	ID3D11Buffer* pSkyboxCBuffer;
	ID3D11Buffer* pCharacterCBuffer;


	//Depth Buffer
	ID3D11Texture2D* pDepthBuffer;
	ID3D11DepthStencilView* pDSV;
	//ID3D11DepthStencilState* pDSLessEqual;


	
	//Create Timer
	XTime timer;
	float dt;


	float translateZ = 0.0f;
	float translateY = 0.0f;
	float translateX = 0.0f;
	float rotationY = 0.0f;
	float rotationX = 0.0f;

	//Create the shaders
	
	//Basic shaders
	ID3D11VertexShader* pVertexShader = nullptr;
	ID3D11PixelShader* pPixelShader = nullptr;

	//Skybox Shaders
	ID3D11VertexShader* pSkybox_VS = nullptr;
	ID3D11PixelShader* pSkybox_PS = nullptr;
	//ID3D11ShaderResourceView* pSBResourceView;

	ID3D11RasterizerState* pRSCullNone;
	ID3D11RasterizerState* pRSDefault;


	//Constant buffer for rendered object
	struct OBJECT
	{
		XMFLOAT4X4 worldMatrix;
	};

	//constant buffer for the scene
	struct SCENE
	{
		XMFLOAT4X4 viewMatrix;
		XMFLOAT4X4 projMatrix;
	};

	OBJECT star;
	SCENE scene;
	OBJECT SkyBox;
	OBJECT character;


	//Model loading variables
	


	//MULTITHREADED LOADING VARIABLES
	unsigned int numModels = 2;
	unsigned int numModelsLoaded = 0;
	mutex ModelsLoadedMutex;
	condition_variable modelCountCondition;

	//MULTITHREADED RENDERING VARIABLES
	ID3D11CommandList* pSBCommandList = nullptr;
	ID3D11CommandList* pGeometryCommandList;
	unsigned int numDrawCalls = 0;
	unsigned int numToDraw = 1;
	mutex DrawCallsMutex;
	condition_variable DrawCountCondition;

public:
	//Create the application and the functions it will use to run and shutdown
	APPLICATION(HINSTANCE hinst, WNDPROC proc);
	bool Run();
	void CheckInput();
	bool ShutDown();

	struct SIMPLE_VERTEX
	{
		XMFLOAT3 position;
		XMFLOAT4 rgba;
		XMFLOAT3 normal;
		XMFLOAT2 UV;
		
	};

	bool LoadFBX(vector<SIMPLE_VERTEX>* output, string filename, int numVerts);
	static void ModelThreadEntry(APPLICATION* which);
	static void TextureThreadEntry(APPLICATION* which);
	static void SBDrawThreadEntry(APPLICATION* which);
	static void GeometryDrawThreadEntry(APPLICATION* which);


};

/******************************************************************/
/************************* INITIALIZATION *************************/
APPLICATION::APPLICATION(HINSTANCE hinst, WNDPROC proc)
{
	//BEGIN WINDOWS CODE
	application = hinst;
	appWndProc = proc;

	WNDCLASSEX wndClass;
	ZeroMemory(&wndClass, sizeof(wndClass));
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.lpfnWndProc = appWndProc;
	wndClass.lpszClassName = L"DirectXApplication";
	wndClass.hInstance = application;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOWFRAME);
	//wndClass.hIcon = LoadICon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_FSICON));
	RegisterClassEx(&wndClass);

	RECT window_size = { 0, 0, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT };
	AdjustWindowRect(&window_size, WS_OVERLAPPEDWINDOW, false);

	window = CreateWindow(L"DirectXApplication", L"Graphics 2 Project", WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX),
		CW_USEDEFAULT, CW_USEDEFAULT, window_size.right - window_size.left, window_size.bottom - window_size.top,
		NULL, NULL, application, this);

	ShowWindow(window, SW_SHOW);

	//END WINDOWS CODE


	//Create device and swap chain
	DXGI_SWAP_CHAIN_DESC SwapChainDesc;
	ZeroMemory(&SwapChainDesc, sizeof(SwapChainDesc));
	D3D_FEATURE_LEVEL* pFLevel = nullptr;


	//Setup the swap chain description
	SwapChainDesc.BufferCount = 1;
	SwapChainDesc.BufferDesc.Width = BACKBUFFER_WIDTH;
	SwapChainDesc.BufferDesc.Height = BACKBUFFER_HEIGHT;
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	SwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	SwapChainDesc.OutputWindow = window;
	SwapChainDesc.SampleDesc.Count = 4;
	SwapChainDesc.SampleDesc.Quality = 8;
	SwapChainDesc.Windowed = true;

	//Array of feature levels
	D3D_FEATURE_LEVEL pFeatureLevels[6] =
	{
		D3D_FEATURE_LEVEL_9_1,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_11_0
	};


	unsigned int flag = 0;

#ifdef _DEBUG
	flag |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE, NULL, flag,
		0, 0, D3D11_SDK_VERSION, &SwapChainDesc,
		&pSwapChain, &pDevice, 0, &pDeviceContext
		);


	hr = pDevice->CreateDeferredContext(NULL, &pDeferredContext);
	hr = pDevice->CreateDeferredContext(NULL, &pGeoDeferredContext);

	//set the buffer
	hr = pSwapChain->GetBuffer(0, __uuidof(pBackBuffer), reinterpret_cast<void**>(&pBackBuffer));
	
	//Create the render target view
	hr = pDevice->CreateRenderTargetView(pBackBuffer, NULL, &pRTV);


	//Set viewport data
	vp.Height = BACKBUFFER_HEIGHT;
	vp.Width = BACKBUFFER_WIDTH;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;


	//MULTITHREADED LOADING OF MODELS 
	//Create the skybox
	thread tempThreadModel(ModelThreadEntry, this);
	tempThreadModel.detach();

	//MULTITHREADED LOADING OF TEXTURES
	//Create Knight model
	thread tempThreadTexture(TextureThreadEntry, this);
	tempThreadTexture.detach();

	//WAIT FOR ALL MODELS AND TEXTURES TO BE LOADED BEFORE CONTINUING
	unique_lock<mutex> ulCountCondition(ModelsLoadedMutex);
	modelCountCondition.wait(ulCountCondition, [&](){
		return numModelsLoaded == numModels;
	});
	ulCountCondition.unlock();


	//Create the sample state description
	D3D11_SAMPLER_DESC descSampleState;
	ZeroMemory(&descSampleState, sizeof(descSampleState));
	descSampleState.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	descSampleState.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	descSampleState.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	descSampleState.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	descSampleState.MipLODBias = 0.0f;
	descSampleState.MaxAnisotropy = 1;
	descSampleState.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	descSampleState.BorderColor[0] = 0;
	descSampleState.BorderColor[1] = 0;
	descSampleState.BorderColor[2] = 0;
	descSampleState.BorderColor[3] = 0;
	descSampleState.MinLOD = 0;
	descSampleState.MaxLOD = D3D11_FLOAT32_MAX;

	//Use the description to make the sample state
	hr = pDevice->CreateSamplerState(&descSampleState, &pSamplerState);


	D3D11_RASTERIZER_DESC descRaster;
	ZeroMemory(&descRaster, sizeof(descRaster));
	descRaster.FillMode = D3D11_FILL_SOLID;
	descRaster.CullMode = D3D11_CULL_NONE;
	descRaster.FrontCounterClockwise = true;
	descRaster.DepthClipEnable = true;

	hr = pDevice->CreateRasterizerState(&descRaster, &pRSCullNone);

	D3D11_RASTERIZER_DESC descDefRaster;
	ZeroMemory(&descDefRaster, sizeof(descRaster));
	descRaster.FillMode = D3D11_FILL_SOLID;
	descRaster.CullMode = D3D11_CULL_NONE;
	descRaster.FrontCounterClockwise = false;
	descRaster.DepthClipEnable = true;

	hr = pDevice->CreateRasterizerState(&descRaster, &pRSDefault);

	for(int i = 0; i < SkyBoxVerts.size(); i++)
	{

		if(i < SkyBoxVerts.size()/2)
		{
		SkyBoxVerts[i].rgba.x = 0.1f;
		SkyBoxVerts[i].rgba.y = 1.0f;
		SkyBoxVerts[i].rgba.z = 0.1f;
		SkyBoxVerts[i].rgba.w = 1.0f;
		}
		else
		{
			SkyBoxVerts[i].rgba.x = 1.0f;
			SkyBoxVerts[i].rgba.y = 1.0f;
			SkyBoxVerts[i].rgba.z = 1.0f;
			SkyBoxVerts[i].rgba.w = 1.0f;
		}

	}


	//Describe the Skybox vert buffer
	D3D11_BUFFER_DESC descSBVertBuffer;
	ZeroMemory(&descSBVertBuffer, sizeof(descSBVertBuffer));
	descSBVertBuffer.Usage = D3D11_USAGE_IMMUTABLE;
	descSBVertBuffer.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	descSBVertBuffer.CPUAccessFlags = NULL;
	descSBVertBuffer.ByteWidth = sizeof(SIMPLE_VERTEX)* 2280;



	//Create buffer initial data
	D3D11_SUBRESOURCE_DATA data;
	ZeroMemory(&data, sizeof(data));

	//set the memory to the desired data
	data.pSysMem = &SkyBoxVerts[0];
	//create the vertex buffer
	hr = pDevice->CreateBuffer(&descSBVertBuffer, &data, &pSBVertexBuffer);



	//Describe the character vert buffer
	D3D11_BUFFER_DESC descCharVertBuffer;
	ZeroMemory(&descCharVertBuffer, sizeof(descCharVertBuffer));
	descCharVertBuffer.Usage = D3D11_USAGE_IMMUTABLE;
	descCharVertBuffer.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	descCharVertBuffer.CPUAccessFlags = NULL;
	descCharVertBuffer.ByteWidth = sizeof(SIMPLE_VERTEX) * 17544;


	//Create the character vert buffer
	//Create buffer initial data
	D3D11_SUBRESOURCE_DATA chardata;
	ZeroMemory(&chardata, sizeof(chardata));
	chardata.pSysMem = &CharacterVerts[0];
	hr = pDevice->CreateBuffer(&descCharVertBuffer, &chardata, &pCharacterVertexBuffer);

	//Create the shaders
	hr = pDevice->CreateVertexShader(Trivial_VS, sizeof(Trivial_VS), NULL, &pVertexShader);
	hr = pDevice->CreatePixelShader(Trivial_PS, sizeof(Trivial_PS), NULL, &pPixelShader);
	hr = pDevice->CreateVertexShader(Skybox_VS, sizeof(Skybox_VS), NULL, &pSkybox_VS);
	hr = pDevice->CreatePixelShader(Skybox_PS, sizeof(Skybox_PS), NULL, &pSkybox_PS);
	
	//Setup the input layout
	D3D11_INPUT_ELEMENT_DESC vLayout[] = 
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }

	};

	//Create the input layout
	hr = pDevice->CreateInputLayout(vLayout, 4, Trivial_VS, sizeof(Trivial_VS), &pInputLayout);


	//Describe a const buffer for the skybox data
	D3D11_BUFFER_DESC descSkyboxConstBuffer;
	ZeroMemory(&descSkyboxConstBuffer, sizeof(descSkyboxConstBuffer));
	descSkyboxConstBuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	descSkyboxConstBuffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	descSkyboxConstBuffer.Usage = D3D11_USAGE_DYNAMIC;
	descSkyboxConstBuffer.ByteWidth = sizeof(OBJECT);
	//create the object const buffer
	hr = pDevice->CreateBuffer(&descSkyboxConstBuffer, NULL, &pSkyboxCBuffer);

	//Describe a const buffer for the character model data
	D3D11_BUFFER_DESC descCharConstBuffer;
	ZeroMemory(&descCharConstBuffer, sizeof(descCharConstBuffer));
	descCharConstBuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	descCharConstBuffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	descCharConstBuffer.Usage = D3D11_USAGE_DYNAMIC;
	descCharConstBuffer.ByteWidth = sizeof(OBJECT);
	//create the object const buffer
	hr = pDevice->CreateBuffer(&descCharConstBuffer, NULL, &pCharacterCBuffer);

	//Describe the scene constant buffer
	D3D11_BUFFER_DESC descSceneConstBuffer;
	ZeroMemory(&descSceneConstBuffer, sizeof(descSceneConstBuffer));
	descSceneConstBuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	descSceneConstBuffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	descSceneConstBuffer.Usage = D3D11_USAGE_DYNAMIC;
	descSceneConstBuffer.ByteWidth = sizeof(SCENE);

	//Create the scene const buffer
	hr = pDevice->CreateBuffer(&descSceneConstBuffer, NULL, &pSceneCBuffer);
	

	//Describe the depth buffer
	D3D11_TEXTURE2D_DESC descDepthBuffer;
	ZeroMemory(&descDepthBuffer, sizeof(descDepthBuffer));
	descDepthBuffer.Width = BACKBUFFER_WIDTH;
	descDepthBuffer.Height = BACKBUFFER_HEIGHT;
	descDepthBuffer.MipLevels = 1;
	descDepthBuffer.ArraySize = 1;
	descDepthBuffer.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepthBuffer.Format = DXGI_FORMAT_D32_FLOAT;
	descDepthBuffer.SampleDesc.Count = 4;
	descDepthBuffer.SampleDesc.Quality = 8;
	descDepthBuffer.Usage = D3D11_USAGE_DEFAULT;


	//Create the depth buffer
	hr = pDevice->CreateTexture2D(&descDepthBuffer, NULL, &pDepthBuffer);

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	descDSV.Texture2D.MipSlice = 0;


	//Create the depth stencil view
	hr = pDevice->CreateDepthStencilView(pDepthBuffer, &descDSV, &pDSV);


	//CREATE STAR OBJECT VERT BUFFER
	SIMPLE_VERTEX verts[12];

	//Create the star
	for(unsigned int i = 0; i < 10; i++)
	{
		verts[i].position.x = cos(i * ((2 * 3.14159) / 10.0f));
		verts[i].position.y = sin(i * ((2 * 3.14159) / 10.0f));
		verts[i].position.z = 0;

		if(i % 2 != 0)
		{
			verts[i].position.x *= 0.4f;
			verts[i].position.y *= 0.4f;
			verts[i].position.z *= 0.4f;
		}

		verts[i].rgba.x = 1;
		verts[i].rgba.y = 0;
		verts[i].rgba.z = 0;
		verts[i].rgba.w = 1;

	}

	verts[10].position.x = 0;
	verts[10].position.y = 0;
	verts[10].position.z = -.3;
	verts[10].rgba.x = 0;
	verts[10].rgba.y = 0;
	verts[10].rgba.z = 1;
	verts[10].rgba.w = 1;
	verts[11].position.x = 0;
	verts[11].position.y = 0;
	verts[11].position.z = .3;
	verts[11].rgba.x = 0;
	verts[11].rgba.y = 0;
	verts[11].rgba.z = 1;
	verts[11].rgba.w = 1;

	//Describe the back buffer
	D3D11_BUFFER_DESC descStarVertBuffer;
	ZeroMemory(&descStarVertBuffer, sizeof(descStarVertBuffer));
	descStarVertBuffer.Usage = D3D11_USAGE_IMMUTABLE;
	descStarVertBuffer.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	descStarVertBuffer.CPUAccessFlags = NULL;
	descStarVertBuffer.ByteWidth = sizeof(SIMPLE_VERTEX) * TriVerts;

	//Create buffer initial data
	ZeroMemory(&data, sizeof(data));

	//set the memory to the desired data
	//data.pSysMem = verts;
	data.pSysMem = &verts[0];
	//create the vertex buffer
	if(pStarVertBuffer)
	{
		pStarVertBuffer->Release();
		pStarVertBuffer = nullptr;
	}

	hr = pDevice->CreateBuffer(&descStarVertBuffer, &data, &pStarVertBuffer);


	//Setup the star vertex winding
	unsigned int indices[] =
	{ 10, 0, 9,
	10, 9, 8,
	10, 8, 7,
	10, 7, 6,
	10, 6, 5,
	10, 5, 4,
	10, 4, 3,
	10, 3, 2,
	10, 2, 1,
	10, 1, 0,
	11, 0, 1,
	11, 1, 2,
	11, 2, 3,
	11, 3, 4,
	11, 4, 5,
	11, 5, 6,
	11, 6, 7,
	11, 7, 8,
	11, 8, 9,
	11, 9, 0
	};

	//Describe the index buffer
	D3D11_BUFFER_DESC descIndexBuffer;
	ZeroMemory(&descIndexBuffer, sizeof(descIndexBuffer));
	descIndexBuffer.BindFlags = D3D10_BIND_INDEX_BUFFER;
	descIndexBuffer.ByteWidth = sizeof(unsigned int) * TriVerts;
	descIndexBuffer.Usage = D3D11_USAGE_DEFAULT;
	

	//Create the initial data for the index buffer
	D3D11_SUBRESOURCE_DATA indexInitData;
	indexInitData.pSysMem = indices;
	indexInitData.SysMemPitch = 0;
	indexInitData.SysMemSlicePitch = 0;

	//create the index buffer
	hr = pDevice->CreateBuffer(&descIndexBuffer, &indexInitData, &pIndexBuffer);

	//describe the object constant buffer
	D3D11_BUFFER_DESC descObjectConstBuffer;
	ZeroMemory(&descObjectConstBuffer, sizeof(descObjectConstBuffer));
	descObjectConstBuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	descObjectConstBuffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	descObjectConstBuffer.Usage = D3D11_USAGE_DYNAMIC;
	descObjectConstBuffer.ByteWidth = sizeof(OBJECT);

	hr = pDevice->CreateBuffer(&descObjectConstBuffer, NULL, &pObjectCBuffer);

	
	pSBCommandList = nullptr;

	dt = 0;


}

/******************************************************************/
/****************************** Run ******************************/
bool APPLICATION::Run()
{
	timer.Signal();
	dt += timer.Delta();

	//Set Render Target
	pDeferredContext->OMSetRenderTargets(1, &pRTV, pDSV);

	//Set viewports
	pDeferredContext->RSSetViewports(1, &vp);

	//Set screen clear color
	//Blood red
	float clearColor[4] =
	{
		.5f, .01f, .01f, 1
	};

	//Clear Screen
	pDeferredContext->ClearRenderTargetView(pRTV, clearColor);
	pDeferredContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	//Create the star's world matrix 
	star.worldMatrix =
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};
	
	//Create and manipulate the character models world matrix for its rotation and scaling
	XMStoreFloat4x4(&character.worldMatrix, XMMatrixMultiply(XMMatrixIdentity(), XMMatrixScaling(0.1f, 0.1f, 0.1f)));
	XMStoreFloat4x4(&character.worldMatrix, XMMatrixRotationY(dt));
	
	//Rotate world matrix on y axis
	XMStoreFloat4x4(&star.worldMatrix, XMMatrixRotationY(dt));


	//Create the scene's view matrix 
	scene.viewMatrix =
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	//Create the view matrix
	XMMATRIX sceneViewMat = XMLoadFloat4x4(&scene.viewMatrix);
	XMVECTOR sceneViewDet = XMMatrixDeterminant(sceneViewMat);
	


	
	//Check for input to move the camera
	CheckInput();
	
	//Apply camera rotations
	sceneViewMat = XMMatrixMultiply(sceneViewMat, XMMatrixRotationY(rotationY));
	sceneViewMat = XMMatrixMultiply(sceneViewMat, XMMatrixRotationX(rotationX));

	//Apply camera translations
	sceneViewMat = XMMatrixMultiply(sceneViewMat, XMMatrixTranslation(translateX, translateY, translateZ));
	

	XMStoreFloat4x4(&scene.viewMatrix, XMMatrixInverse(&sceneViewDet, sceneViewMat));

	FLOAT aspect_ratio = BACKBUFFER_WIDTH / (float)BACKBUFFER_HEIGHT;

	XMStoreFloat4x4( &scene.projMatrix,XMMatrixPerspectiveFovLH(XMConvertToRadians(65.0f), aspect_ratio, 0.01f, 1000.0f));

	XMStoreFloat4x4(&SkyBox.worldMatrix, XMMatrixTranslation(translateX, translateY, translateZ));
	

	//Memcpy data from const buffer structs into Vertex shader const buffers
	//Takes data from cpu to gpu
	//Object 
	D3D11_MAPPED_SUBRESOURCE pSubRes;
	pDeferredContext->Map(pSkyboxCBuffer, 0, D3D11_MAP_WRITE_DISCARD,
		0, &pSubRes);
	memcpy(pSubRes.pData, &SkyBox, sizeof(OBJECT));
	pDeferredContext->Unmap(pSkyboxCBuffer, 0);


	//Do the same for the scene
	D3D11_MAPPED_SUBRESOURCE pSubResScene;
	pDeferredContext->Map(pSceneCBuffer, 0, D3D11_MAP_WRITE_DISCARD,
		0, &pSubResScene);
	memcpy(pSubResScene.pData, &scene, sizeof(SCENE));
	pDeferredContext->Unmap(pSceneCBuffer, 0);

	pDeferredContext->VSSetConstantBuffers(0, 1, &pSkyboxCBuffer);
	pDeferredContext->VSSetConstantBuffers(1, 1, &pSceneCBuffer);

	/*thread SBRenderThread(SBDrawThreadEntry, this);
	SBRenderThread.detach();*/

	////WAIT FOR ALL MODELS AND TEXTURES TO BE LOADED BEFORE CONTINUING
	//unique_lock<mutex> ulCountCondition(DrawCallsMutex);
	//modelCountCondition.wait(ulCountCondition, [&](){
	//	return numDrawCalls == numToDraw;
	//});
	//ulCountCondition.unlock();
	
	////Link buffers

	//
	//
	////Set the vertex buffer for our object
	unsigned int stride[] = { sizeof(SIMPLE_VERTEX) };
	unsigned int offsets[] = { 0 };
	pDeferredContext->IASetVertexBuffers(0, 1, &pSBVertexBuffer, stride, offsets);

	//Set the shaders to be used
	pDeferredContext->VSSetShader(pSkybox_VS, NULL, 0);
	pDeferredContext->PSSetShader(pSkybox_PS, NULL, 0);
	pDeferredContext->PSSetShaderResources(0, 1, &pSRView);
	pDeferredContext->PSSetSamplers(0, 1, &pSamplerState);

	//Set the input layout to be used
	pDeferredContext->IASetInputLayout(pInputLayout);

	//Set the type of primitive topology to be used
	pDeferredContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pDeferredContext->RSSetState(this->pRSCullNone);
	pDeferredContext->Draw(2280, 0);

	pDeferredContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0f, NULL);
	pDeferredContext->FinishCommandList(true, &pSBCommandList);
	//Draw the object	
	if(pSBCommandList)
	{
		pDeviceContext->ExecuteCommandList(pSBCommandList, false);
		pSBCommandList->Release();
		pSBCommandList = nullptr;
	}



	//Set values for star draw call
	//Set Render Target
	pGeoDeferredContext->OMSetRenderTargets(1, &pRTV, pDSV);

	//Set viewports
	pGeoDeferredContext->RSSetViewports(1, &vp);

	pGeoDeferredContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	pGeoDeferredContext->IASetVertexBuffers(0, 1, &pStarVertBuffer, stride, offsets);

	//Memcpy the stars data from cpu to gpu
	D3D11_MAPPED_SUBRESOURCE pStarSubRes;
	pGeoDeferredContext->Map(pObjectCBuffer, 0, D3D11_MAP_WRITE_DISCARD,
		0, &pStarSubRes);
	memcpy(pStarSubRes.pData, &star, sizeof(OBJECT));
	pGeoDeferredContext->Unmap(pObjectCBuffer, 0);

	//set the star's constant buffer
	pGeoDeferredContext->VSSetConstantBuffers(0, 1, &pObjectCBuffer);
	pGeoDeferredContext->VSSetConstantBuffers(1, 1, &pSceneCBuffer);
	
	//Change shaders 
	pGeoDeferredContext->VSSetShader(pVertexShader, NULL, 0);
	pGeoDeferredContext->PSSetShader(pPixelShader, NULL, 0);
	pGeoDeferredContext->PSSetShaderResources(0, 1, &NullSRV);
	pGeoDeferredContext->PSSetSamplers(0, 1, &pSamplerState);
	//Input layout
	pGeoDeferredContext->IASetInputLayout(pInputLayout);
	//Rasterizer State
	pGeoDeferredContext->RSSetState(pRSDefault);
	//Primitive topology
	pGeoDeferredContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	pGeoDeferredContext->DrawIndexed(TriVerts, 0, 0);

	//Start character model drawing
	//Memcpy the characters data from cpu to gpu
	D3D11_MAPPED_SUBRESOURCE CharSubRes;
	pGeoDeferredContext->Map(pCharacterCBuffer, 0, D3D11_MAP_WRITE_DISCARD,
		0, &CharSubRes);
	memcpy(CharSubRes.pData, &character, sizeof(OBJECT));
	pGeoDeferredContext->Unmap(pCharacterCBuffer, 0);

	//Set values for drawing for new model
	pGeoDeferredContext->PSSetShaderResources(0, 1, &pCharacterSRV);
	pGeoDeferredContext->VSSetConstantBuffers(0, 1, &pCharacterCBuffer);
	pGeoDeferredContext->VSSetConstantBuffers(1, 1, &pSceneCBuffer);

	pGeoDeferredContext->IASetVertexBuffers(0, 1, &pCharacterVertexBuffer, stride, offsets);
	pGeoDeferredContext->Draw(17544, 0);

	pGeoDeferredContext->FinishCommandList(false, &pGeometryCommandList);

	pDeviceContext->ExecuteCommandList(pGeometryCommandList, false);
	pGeometryCommandList->Release();
	pGeometryCommandList = nullptr;
	
	//Present to the screen
	pSwapChain->Present(0, 0);

	return true;
}

/*****************************************************************/
/************************** Shutdown ****************************/
bool APPLICATION::ShutDown()
{
	//Release all memory
	pDeviceContext->ClearState();

	pDeviceContext->Release();
	pRTV->Release();
	pSwapChain->Release();
	pBackBuffer->Release();
	pStarVertBuffer->Release();
	pInputLayout->Release();
	pSBVertexBuffer->Release();
	pObjectCBuffer->Release();
	pSceneCBuffer->Release();
	pIndexBuffer->Release();
	pSkyboxCBuffer->Release();
	pDepthBuffer->Release();
	pDSV->Release();
	pPixelShader->Release();
	pVertexShader->Release();
	pSRView->Release();
	pSamplerState->Release();
	pSkybox_PS->Release();
	pSkybox_VS->Release();
	pRSCullNone->Release();
	pCharacterCBuffer->Release();
	pCharacterSRV->Release();
	pCharacterVertexBuffer->Release();
	pRSDefault->Release();
	pDeferredContext->Release();

	
	

//	pDSLessEqual->Release();

	pDevice->Release();
	UnregisterClass(L"DirectXApplication", application);

	return true;
}

/***************************************************************/
/************************ MORE WINDOWS ************************/
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wparam, LPARAM lparam);
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int)
{


	
	srand(unsigned int(time(0)));
	APPLICATION myApp(hInstance, (WNDPROC)WndProc);
	MSG msg; ZeroMemory(&msg, sizeof(msg));
	while(msg.message != WM_QUIT && myApp.Run())
	{
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	myApp.ShutDown();
	return 0;
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if(GetAsyncKeyState(VK_ESCAPE))
		message = WM_DESTROY;
	switch(message)
	{
	case (WM_DESTROY) : { PostQuitMessage(0); }
						break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}
/**************************************************************/
/*********************** End Windows *************************/

//Check for input
void APPLICATION::CheckInput()
{
	if(GetAsyncKeyState('W'))
	{
		translateZ += 1 * timer.Delta();
	}
	else if(GetAsyncKeyState('S'))
	{
		translateZ -= 1 * timer.Delta();
	}
	if(GetAsyncKeyState('A'))
	{
		translateX -= 1 * timer.Delta();
	}
	else if(GetAsyncKeyState('D'))
	{
		translateX += 1 * timer.Delta();
	}
	if(GetAsyncKeyState(VK_SPACE))
	{
		translateY += 2 * timer.Delta();
	}
	else if(GetAsyncKeyState('X'))
	{
		translateY -= 2 * timer.Delta();
	}

	if(GetAsyncKeyState('Q'))
	{
		rotationY -= 2 * timer.Delta();
	}
	else if(GetAsyncKeyState('E'))
	{
		rotationY += 2 * timer.Delta();
	}
	if(GetAsyncKeyState(VK_UP))
	{
		rotationX -= 2 * timer.Delta();
	}
	else if(GetAsyncKeyState(VK_DOWN))
	{
		rotationX += 2 * timer.Delta();
	}
}


bool APPLICATION::LoadFBX(vector<APPLICATION::SIMPLE_VERTEX>* output, string filename, int numVerts)
{
	FbxManager* pFBXManager = nullptr;
		pFBXManager = FbxManager::Create();
	//if(pFBXManager == nullptr)
	//{

		FbxIOSettings* pIOSettings = FbxIOSettings::Create(pFBXManager, IOSROOT);
		pFBXManager->SetIOSettings(pIOSettings);

	//}

	FbxImporter* pImporter = FbxImporter::Create(pFBXManager, "");
	FbxScene* pScene = FbxScene::Create(pFBXManager, "");

	bool bSuccess = pImporter->Initialize(filename.c_str(),
		-1, pFBXManager->GetIOSettings());

	if(bSuccess == false)
	{
		return false;
	}

	bSuccess = pImporter->Import(pScene);

	if(bSuccess == false)
	{
		return false;
	}

	pImporter->Destroy();

	FbxNode* pRootNode = pScene->GetRootNode();

	if(pRootNode)
	{

		for(unsigned int i = 0; i < pRootNode->GetChildCount(); i++)
		{
			FbxNode* pChildNode = pRootNode->GetChild(i);

			if(pChildNode->GetNodeAttribute() == NULL)
			{
				continue;
			}

			FbxNodeAttribute::EType AttributeType = pChildNode->GetNodeAttribute()->GetAttributeType();
			
			if(AttributeType != FbxNodeAttribute::eMesh)
			{
				continue;
			}

			FbxMesh* pMesh = (FbxMesh*)pChildNode->GetNodeAttribute();

			FbxVector4* pVertices = pMesh->GetControlPoints();


			for(unsigned int j = 0; j < pMesh->GetPolygonCount(); j++)
			{
				int nNumVertices = pMesh->GetPolygonSize(j);

				assert(nNumVertices == 3 &&
					"LoadFBX function failed due to a vert count not equal to 3. Polygons must be triangles");
				
				for(unsigned int k = 0; k < nNumVertices; k++)
				{
					int nControlPointIndex = pMesh->GetPolygonVertex(j, k);
					
					SIMPLE_VERTEX vert;
					//Set vert positions
					vert.position.x = (float)pVertices[nControlPointIndex].mData[0];
					vert.position.y = (float)pVertices[nControlPointIndex].mData[1];
					vert.position.z = (float)pVertices[nControlPointIndex].mData[2];

					//create variables to get the vert normals
					FbxVector4 normal;
					pMesh->GetPolygonVertexNormal(j, k, normal);
					
					//Set the vert normals
					vert.normal.x = normal.mData[0];
					vert.normal.y = normal.mData[1];
					vert.normal.z = normal.mData[2];

					FbxStringList UVSetNameList;
					pMesh->GetUVSetNames(UVSetNameList);

					const char* UVSetName = UVSetNameList.GetStringAt(0);
					const FbxGeometryElementUV* UVElement = pMesh->GetElementUV(UVSetName);

					if(!UVElement)
					{
						continue;
					}

					if(UVElement->GetMappingMode() != FbxGeometryElement::eByPolygonVertex &&
						UVElement->GetMappingMode() != FbxGeometryElement::eByControlPoint)
					{
						return false;
					}

					const bool UseIndex = UVElement->GetReferenceMode() != FbxGeometryElement::eDirect;
					const int IndexCount = (UseIndex) ? UVElement->GetIndexArray().GetCount() : 0;


					FbxVector2 UV;

					int UVIndex = pMesh->GetTextureUVIndex(j, k);
					UV = UVElement->GetDirectArray().GetAt(UVIndex);

					vert.UV.x = UV.mData[0];
					vert.UV.y = 1.0f - UV.mData[1];

					output->push_back(vert);
				}
			}

		}
	}

	return true;
}


void APPLICATION::ModelThreadEntry(APPLICATION* which)
{
	
	which->LoadFBX(&which->SkyBoxVerts,
		"..\\Assets\\Sphere.fbx",
		which->SBNumVerts);
	which->LoadFBX(&which->CharacterVerts, 
		"..\\Assets\\Knight.fbx",
		which->SBNumVerts);




	which->ModelsLoadedMutex.lock();
	which->numModelsLoaded++;
	which->modelCountCondition.notify_all();
	which->ModelsLoadedMutex.unlock();
}


void APPLICATION::TextureThreadEntry(APPLICATION* which)
{
	

	CreateDDSTextureFromFile(which->pDevice, L"../Assets/GoldKnight.dds", NULL, &which->pCharacterSRV);
	CreateDDSTextureFromFile(which->pDevice, L"../Assets/Skybox.dds", NULL, &which->pSRView);


	which->ModelsLoadedMutex.lock();
	which->numModelsLoaded++;
	which->modelCountCondition.notify_all();
	which->ModelsLoadedMutex.unlock();
}

void APPLICATION::SBDrawThreadEntry(APPLICATION* which)
{

	//Memcpy data from const buffer structs into Vertex shader const buffers
	//Takes data from cpu to gpu
	//Object 

	if(which->pSBCommandList)
		return;



	//Set the vertex buffer for our object
	unsigned int stride[] = { sizeof(SIMPLE_VERTEX) };
	unsigned int offsets[] = { 0 };
	which->pDeferredContext->IASetVertexBuffers(0, 1, &which->pSBVertexBuffer, stride, offsets);

	//Set the shaders to be used
	which->pDeferredContext->VSSetShader(which->pSkybox_VS, NULL, 0);
	which->pDeferredContext->PSSetShader(which->pSkybox_PS, NULL, 0);
	which->pDeferredContext->PSSetShaderResources(0, 1, &which->pSRView);
	which->pDeferredContext->PSSetSamplers(0, 1, &which->pSamplerState);

	//Set the input layout to be used
	which->pDeferredContext->IASetInputLayout(which->pInputLayout);

	//Set the type of primitive topology to be used
	which->pDeferredContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	which->pDeferredContext->RSSetState(which->pRSCullNone);

	//Draw the object	
	which->pDeferredContext->Draw(2280, 0);

	which->pDeferredContext->ClearDepthStencilView(which->pDSV, D3D11_CLEAR_DEPTH, 1.0f, NULL);

	which->pDeferredContext->FinishCommandList(false, &which->pSBCommandList);

	//which->DrawCallsMutex.lock();
	//which->numDrawCalls++;
	//which->DrawCountCondition.notify_all();
	//which->DrawCallsMutex.unlock();
}

void APPLICATION::GeometryDrawThreadEntry(APPLICATION* which)
{


}