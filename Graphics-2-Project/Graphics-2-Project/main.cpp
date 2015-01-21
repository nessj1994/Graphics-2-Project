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
	ID3D11RenderTargetView* pRTV;
	IDXGISwapChain* pSwapChain = nullptr;
	ID3D11Resource* pBackBuffer;
	D3D11_VIEWPORT vp;
	ID3D11Texture2D* pSBTexture;
	ID3D11ShaderResourceView* pSRView;
	ID3D11SamplerState* pSamplerState;
	ID3D11ShaderResourceView* pCharacterSRV;

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
	ID3D11DepthStencilState* pDSLessEqual;


	
	//Create Timer
	XTime timer;
	float dt;


	float translateZ = 0.0f;
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
	FbxManager* pFBXManager = nullptr;


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
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
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


	unsigned int flag = D3D11_CREATE_DEVICE_SINGLETHREADED;

#ifdef _DEBUG
	flag |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE, NULL, flag,
		0, 0, D3D11_SDK_VERSION, &SwapChainDesc,
		&pSwapChain, &pDevice, 0, &pDeviceContext
		);

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

	




	//Create the skybox
	
	LoadFBX(&SkyBoxVerts,
		"..\\Assets\\Sphere.fbx",
		SBNumVerts);
	

	hr = CreateDDSTextureFromFile(pDevice, L"../Assets/Skybox.dds", NULL, &pSRView);
	
	LoadFBX(&CharacterVerts, "..\\Assets\\Jinx.fbx", SBNumVerts);

	hr = CreateDDSTextureFromFile(pDevice, L"../Assets/Jinx.dds", NULL, &pCharacterSRV);

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
	pDevice->CreateSamplerState(&descSampleState, &pSamplerState);


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
	ZeroMemory(&descSBVertBuffer, sizeof(descCharVertBuffer));
	descCharVertBuffer.Usage = D3D11_USAGE_IMMUTABLE;
	descCharVertBuffer.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	descCharVertBuffer.CPUAccessFlags = NULL;
	descCharVertBuffer.ByteWidth = sizeof(SIMPLE_VERTEX) * 50000;


	//Create the character vert buffer
	//Create buffer initial data
	D3D11_SUBRESOURCE_DATA chardata;
	ZeroMemory(&chardata, sizeof(chardata));
	data.pSysMem = &CharacterVerts;
	hr = pDevice->CreateBuffer(&descCharVertBuffer, &chardata, &pCharacterVertexBuffer);

	//Create the shaders
	pDevice->CreateVertexShader(Trivial_VS, sizeof(Trivial_VS), NULL, &pVertexShader);
	pDevice->CreatePixelShader(Trivial_PS, sizeof(Trivial_PS), NULL, &pPixelShader);
	pDevice->CreateVertexShader(Skybox_VS, sizeof(Skybox_VS), NULL, &pSkybox_VS);
	pDevice->CreatePixelShader(Skybox_PS, sizeof(Skybox_PS), NULL, &pSkybox_PS);
	
	//Setup the input layout
	D3D11_INPUT_ELEMENT_DESC vLayout[] = 
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }

	};

	//Create the input layout
	pDevice->CreateInputLayout(vLayout, 4, Trivial_VS, sizeof(Trivial_VS), &pInputLayout);



	D3D11_BUFFER_DESC descSkyboxConstBuffer;
	ZeroMemory(&descSkyboxConstBuffer, sizeof(descSkyboxConstBuffer));
	descSkyboxConstBuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	descSkyboxConstBuffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	descSkyboxConstBuffer.Usage = D3D11_USAGE_DYNAMIC;
	descSkyboxConstBuffer.ByteWidth = sizeof(OBJECT);
	//create the object const buffer
	pDevice->CreateBuffer(&descSkyboxConstBuffer, NULL, &pSkyboxCBuffer);


	D3D11_BUFFER_DESC descCharConstBuffer;
	ZeroMemory(&descCharConstBuffer, sizeof(descCharConstBuffer));
	descCharConstBuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	descCharConstBuffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	descCharConstBuffer.Usage = D3D11_USAGE_DYNAMIC;
	descCharConstBuffer.ByteWidth = sizeof(OBJECT);
	//create the object const buffer
	pDevice->CreateBuffer(&descCharConstBuffer, NULL, &pCharacterCBuffer);

	//Describe the scene constant buffer
	D3D11_BUFFER_DESC descSceneConstBuffer;
	ZeroMemory(&descSceneConstBuffer, sizeof(descSceneConstBuffer));
	descSceneConstBuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	descSceneConstBuffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	descSceneConstBuffer.Usage = D3D11_USAGE_DYNAMIC;
	descSceneConstBuffer.ByteWidth = sizeof(SCENE);

	//Create the scene const buffer
	pDevice->CreateBuffer(&descSceneConstBuffer, NULL, &pSceneCBuffer);
	

	//Describe the depth buffer
	D3D11_TEXTURE2D_DESC descDepthBuffer;
	ZeroMemory(&descDepthBuffer, sizeof(descDepthBuffer));
	descDepthBuffer.Width = BACKBUFFER_WIDTH;
	descDepthBuffer.Height = BACKBUFFER_HEIGHT;
	descDepthBuffer.MipLevels = 1;
	descDepthBuffer.ArraySize = 1;
	descDepthBuffer.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepthBuffer.Format = DXGI_FORMAT_D32_FLOAT;
	descDepthBuffer.SampleDesc.Count = 1;
	descDepthBuffer.SampleDesc.Quality = 0;
	descDepthBuffer.Usage = D3D11_USAGE_DEFAULT;


	//Create the depth buffer
	pDevice->CreateTexture2D(&descDepthBuffer, NULL, &pDepthBuffer);

	pDevice->CreateDepthStencilView(pDepthBuffer, NULL, &pDSV);


	//CREATE STAR OBJECT

	//Create the depth stencil view

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

	pDevice->CreateBuffer(&descStarVertBuffer, &data, &pStarVertBuffer);


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
	pDevice->CreateBuffer(&descIndexBuffer, &indexInitData, &pIndexBuffer);

	//describe the object constant buffer
	D3D11_BUFFER_DESC descObjectConstBuffer;
	ZeroMemory(&descObjectConstBuffer, sizeof(descObjectConstBuffer));
	descObjectConstBuffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	descObjectConstBuffer.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	descObjectConstBuffer.Usage = D3D11_USAGE_DYNAMIC;
	descObjectConstBuffer.ByteWidth = sizeof(OBJECT);

	pDevice->CreateBuffer(&descObjectConstBuffer, NULL, &pObjectCBuffer);

	


	dt = 0;


}

/******************************************************************/
/****************************** Run ******************************/
bool APPLICATION::Run()
{
	timer.Signal();
	dt += timer.Delta();

	//Set Render Target
	pDeviceContext->OMSetRenderTargets(1, &pRTV, pDSV);

	//Set viewports
	pDeviceContext->RSSetViewports(1, &vp);

	//Set screen clear color
	//Blood red
	float clearColor[4] =
	{
		.5f, .01f, .01f, 1
	};

	//Clear Screen
	pDeviceContext->ClearRenderTargetView(pRTV, clearColor);
	pDeviceContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	//Create the star's world matrix 
	star.worldMatrix =
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};
	
	XMStoreFloat4x4(&character.worldMatrix, XMMatrixIdentity());
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
	sceneViewMat = XMMatrixMultiply(sceneViewMat, XMMatrixTranslation(translateX, 0.0f, translateZ));
	

	XMStoreFloat4x4(&scene.viewMatrix, XMMatrixInverse(&sceneViewDet, sceneViewMat));

	FLOAT aspect_ratio = BACKBUFFER_WIDTH / (float)BACKBUFFER_HEIGHT;

	XMStoreFloat4x4( &scene.projMatrix,XMMatrixPerspectiveFovLH(XMConvertToRadians(65.0f), aspect_ratio, 0.01f, 1000.0f));

	XMStoreFloat4x4(&SkyBox.worldMatrix, XMMatrixTranslation(translateX, 0.0f, translateZ));
	


	//Memcpy data from const buffer structs into Vertex shader const buffers
	//Takes data from cpu to gpu
	//Object 
	D3D11_MAPPED_SUBRESOURCE pSubRes;
	pDeviceContext->Map(pSkyboxCBuffer, 0, D3D11_MAP_WRITE_DISCARD,
		0, &pSubRes);
	memcpy(pSubRes.pData, &SkyBox, sizeof(OBJECT));
	pDeviceContext->Unmap(pSkyboxCBuffer, 0);


	//Do the same for the scene
	D3D11_MAPPED_SUBRESOURCE pSubResScene;
	pDeviceContext->Map(pSceneCBuffer, 0, D3D11_MAP_WRITE_DISCARD,
		0, &pSubResScene);
	memcpy(pSubResScene.pData, &scene, sizeof(SCENE));
	pDeviceContext->Unmap(pSceneCBuffer, 0);

	//Link buffers
	pDeviceContext->VSSetConstantBuffers(0, 1, &pSkyboxCBuffer);
	pDeviceContext->VSSetConstantBuffers(1, 1, &pSceneCBuffer);

	//Set the vertex buffer for our object
	unsigned int stride[] = { sizeof(SIMPLE_VERTEX) };
	unsigned int offsets[] = { 0 };
	pDeviceContext->IASetVertexBuffers(0, 1, &pSBVertexBuffer, stride, offsets);

	//Set the shaders to be used
	pDeviceContext->VSSetShader(pSkybox_VS, NULL, 0);
	pDeviceContext->PSSetShader(pSkybox_PS, NULL, 0);
	pDeviceContext->PSSetShaderResources(0, 1, &pSRView);
	pDeviceContext->PSSetSamplers(0, 1, &pSamplerState);

	//Set the input layout to be used
	pDeviceContext->IASetInputLayout(pInputLayout);

	//Set the type of primitive topology to be used
	pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pDeviceContext->RSSetState(this->pRSCullNone);

	//Draw the object	
	pDeviceContext->Draw(2280, 0);

	pDeviceContext->ClearDepthStencilView(pDSV, D3D11_CLEAR_DEPTH, 1.0f, NULL);


	//Set values for star draw call

	pDeviceContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	pDeviceContext->IASetVertexBuffers(0, 1, &pStarVertBuffer, stride, offsets);

	//Memcpy the stars data from cpu to gpu
	D3D11_MAPPED_SUBRESOURCE pStarSubRes;
	pDeviceContext->Map(pObjectCBuffer, 0, D3D11_MAP_WRITE_DISCARD,
		0, &pStarSubRes);
	memcpy(pStarSubRes.pData, &star, sizeof(OBJECT));
	pDeviceContext->Unmap(pObjectCBuffer, 0);

	//Create the star's constant buffer

	pDeviceContext->VSSetConstantBuffers(0, 1, &pObjectCBuffer);

	//Change shaders 
	pDeviceContext->VSSetShader(pVertexShader, NULL, 0);
	pDeviceContext->PSSetShader(pPixelShader, NULL, 0);
	ID3D11ShaderResourceView* NullSRV = nullptr;
	pDeviceContext->PSSetShaderResources(0, 1, &NullSRV);
	pDeviceContext->RSSetState(pRSDefault);


	pDeviceContext->DrawIndexed(TriVerts, 0, 0);

	//Start character model drawing

	//Memcpy the characters data from cpu to gpu
	D3D11_MAPPED_SUBRESOURCE CharSubRes;
	pDeviceContext->Map(pCharacterCBuffer, 0, D3D11_MAP_WRITE_DISCARD,
		0, &CharSubRes);
	memcpy(CharSubRes.pData, &character, sizeof(OBJECT));
	pDeviceContext->Unmap(pCharacterCBuffer, 0);

	//Set values for drawing for new model
	pDeviceContext->PSSetShaderResources(0, 1, &pCharacterSRV);
	pDeviceContext->VSGetConstantBuffers(0, 1, &pCharacterCBuffer);
	pDeviceContext->IASetVertexBuffers(0, 1, &pCharacterVertexBuffer, stride, offsets);
	pDeviceContext->Draw(50000, 0);

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

	if(pFBXManager == nullptr)
	{
		pFBXManager = FbxManager::Create();

		FbxIOSettings* pIOSettings = FbxIOSettings::Create(pFBXManager, IOSROOT);
		pFBXManager->SetIOSettings(pIOSettings);

	}

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
