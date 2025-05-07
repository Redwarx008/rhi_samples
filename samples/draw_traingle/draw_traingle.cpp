#include <memory>
#include <iostream>
#include <fstream>
#include <vector>
#include <string_view>

#include <rhi/rhi_cpp.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32 1
#include <GLFW/glfw3native.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.hpp"

using namespace rhi;


static void loggingCallback(LoggingSeverity severity, const char* msg)
{
	std::cerr << msg;
}

static std::vector<char> loadShaderData(const char* filePath)
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);
	std::vector<char> buffer;
	if (!file.is_open()) {
		return buffer;
	}

	size_t fileSize = (size_t)file.tellg();
	buffer.resize(fileSize);
	//spirv expects the buffer to be on uint32, so make sure to reserve an int vector big enough for the entire file

	//put file cursor at beginning
	file.seekg(0);

	//load the entire file into the buffer
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

	//now that the file is loaded into the buffer, we can close it
	file.close();

	return buffer;
}

struct Vertex {
	float position[3];
	float color[3];
};

const std::vector<Vertex> vertices{
	{ {  1.0f,  -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
	{ { -1.0f,  -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
	{ {  0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
};

std::vector<uint32_t> indices{ 0, 1, 2 };

struct ShaderData {
	glm::mat4 projectionMatrix;
	glm::mat4 modelMatrix;
	glm::mat4 viewMatrix;
};

Camera camera;

float lastX = 1024 / 2;
float lastY = 1024 / 2;

class App
{
public:
	void init()
	{
		if (!glfwInit())
		{
			std::cerr << "Failed to initialize GLFW\n";
			return;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		mWindow = glfwCreateWindow(mWindowWidth, mWindowHeight, "draw_traingle", nullptr, nullptr);
		if (!mWindow)
		{
			std::cerr << "Failed to create a glfw window\n";
			glfwTerminate();
			return;
		}

		//glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		glfwSetWindowUserPointer(mWindow, this);
		glfwSetKeyCallback(mWindow, &GLFW_KeyCallback);
		glfwSetCursorPosCallback(mWindow, &GLFW_CursorPosCallback);

		// create Instance
		rhi::InstanceDesc instanceDesc{};
		instanceDesc.backend = BackendType::Vulkan;
		instanceDesc.enableDebugLayer = true;
		rhi::Instance instance = rhi::CreateInstance(instanceDesc);

		// create Device
		std::vector<rhi::Adapter> adapters = instance.EnumerateAdapters();
		assert(adapters.size() != 0);

		rhi::DeviceDesc deviceDesc{};
		deviceDesc.name = "Draw traingle";
		mDevice = adapters[0].CreateDevice(deviceDesc);

		// create Surface
		rhi::SurfaceConfiguration surfaceConfig{};
		surfaceConfig.device = mDevice;
		surfaceConfig.width = mWindowWidth;
		surfaceConfig.height = mWindowHeight;
		surfaceConfig.presentMode = PresentMode::Fifo;
		surfaceConfig.format = TextureFormat::BGRA8_SRGB;
		mSurface = instance.CreateSurfaceFromWindowsHWND(glfwGetWin32Window(mWindow), GetModuleHandle(nullptr));
		mSurface.Configure(surfaceConfig);

		mQueue = mDevice.GetQueue(QueueType::Graphics);
		// create depth stencil texture
		rhi::TextureDesc dsDesc{};
		dsDesc.format = TextureFormat::D32_UNORM_S8_UINT;
		dsDesc.width = mWindowWidth;
		dsDesc.height = mWindowHeight;
		dsDesc.dimension = TextureDimension::Texture2D;
		dsDesc.usage = TextureUsage::RenderAttachment;
		mDepthStencilTexture = mDevice.CreateTexture(dsDesc);
		mDsView = mDepthStencilTexture.CreateView();
		// create shader 
		std::vector<char> buffer = loadShaderData("triangle.vert.spv");
		rhi::ShaderModuleDesc shaderCI{};
		shaderCI.type = ShaderStage::Vertex;
		shaderCI.entry = "main";
		shaderCI.code = { buffer.data(), buffer.size() };
		auto vertexShader = mDevice.CreateShader(shaderCI);
		buffer = loadShaderData("triangle.frag.spv");
		shaderCI.type = ShaderStage::Fragment;
		shaderCI.code = { buffer.data(), buffer.size() };
		auto fragmentShader = mDevice.CreateShader(shaderCI);

		// create vertex buffer and index buffer
		BufferDesc bufferDesc{};
		bufferDesc.usage = BufferUsage::Vertex;
		bufferDesc.size = static_cast<uint32_t>(vertices.size()) * sizeof(Vertex);
		bufferDesc.name = "Vertex";
		mVertexBuffer = mDevice.CreateBuffer(bufferDesc);

		bufferDesc.usage = BufferUsage::Index;
		bufferDesc.size = indices.size() * sizeof(uint32_t);
		bufferDesc.name = "Index";
		mIndexBuffer = mDevice.CreateBuffer(bufferDesc);

		bufferDesc.usage = BufferUsage::Uniform;
		bufferDesc.size = sizeof(ShaderData);
		bufferDesc.name = "Uniform";
		mUniformBuffer = mDevice.CreateBuffer(bufferDesc);

		// These match the following shader layout (see triangle.vert):
		//	layout (location = 0) in vec3 inPos;
		//	layout (location = 1) in vec3 inColor;
		VertexInputAttribute vertexInputs[] =
		{
			{0, 0, VertexFormat::Float32x3}, 
			{0, 1, VertexFormat::Float32x3}
		};

		// create Bind set and layout
		BindSetLayoutEntry layoutEntry[] = { BindSetLayoutEntry::UniformBuffer(ShaderStage::Vertex, 0) };

		BindSetEntry entry[] = { BindSetEntry::Buffer(mUniformBuffer, 0) };

		rhi::BindSetLayoutDesc bindSetLayoutDesc{};
		bindSetLayoutDesc.entries = layoutEntry;
		bindSetLayoutDesc.entryCount = 1;
		mBindSetLayout = mDevice.CreateBindSetLayout(bindSetLayoutDesc);
		rhi::BindSetDesc bindSetDesc{};
		bindSetDesc.entries = entry;
		bindSetDesc.entryCount = 1;
		bindSetDesc.layout = mBindSetLayout;
		mBindSet = mDevice.CreateBindSet(bindSetDesc);
		// create pipelineLayout
		rhi::PipelineLayoutDesc desc{};
		desc.bindSetLayouts = &mBindSetLayout;
		desc.bindSetLayoutCount = 1;
		rhi::PipelineLayout pipelineLayout = mDevice.CreatePipelineLayout(desc);

		ShaderState vertexState{ vertexShader };
		ShaderState fragmentState{ fragmentShader };
		// create pipeline
		RenderPipelineDesc pipelineCI{};
		pipelineCI.pipelineLayout = pipelineLayout;
		pipelineCI.vertexAttributes = vertexInputs;
		pipelineCI.vertexAttributeCount = 2;
		pipelineCI.vertexShader = &vertexState;
		pipelineCI.fragmentShader = &fragmentState;
		pipelineCI.colorAttachmentFormats[0] = mSurface.GetSwapChainFormat();
		pipelineCI.colorAttachmentCount = 1;
		pipelineCI.depthStencilFormat = mDepthStencilTexture.GetFormat();
		pipelineCI.depthStencilState.depthTestEnable = true;
		pipelineCI.rasterState.cullMode = CullMode::None;
		pipelineCI.rasterState.frontFace = FrontFace::FrontCounterClockwise;
		pipelineCI.rasterState.primitiveType = PrimitiveType::TriangleList;

		mPipeline = mDevice.CreateRenderPipeline(pipelineCI);

		mCommandEncoder = mDevice.CreateCommandEncoder();

		camera.position = glm::vec3(0, 0, 3);

		// Update the uniform buffer for the next frame
		ShaderData shaderData{};
		camera.update();
		glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)mWindowWidth / (float)mWindowHeight, 10000.f, 0.1f);

		glm::mat4 view = camera.getViewMatrix();
		shaderData.projectionMatrix = projection;
		shaderData.viewMatrix = view;
		shaderData.modelMatrix = glm::mat4(1.0f);


		if (Queue transferQueue = mDevice.GetQueue(QueueType::Transfer))
		{
			transferQueue.WriteBuffer(mIndexBuffer, indices.data(), indices.size() * sizeof(uint32_t), 0);
			transferQueue.WriteBuffer(mVertexBuffer, vertices.data(), vertices.size() * sizeof(Vertex), 0);
			transferQueue.WriteBuffer(mUniformBuffer, &shaderData, sizeof(ShaderData), 0);

			Buffer buffersNeedTransfer[] = { mVertexBuffer, mIndexBuffer, mUniformBuffer };

			ResourceTransfer transfer{};
			transfer.receivingQueue = mQueue;
			transfer.buffers = buffersNeedTransfer;
			transfer.bufferCount = 3;
			uint64_t submitSerial = transferQueue.Submit(nullptr, 0, &transfer, 1);
			mQueue.WaitFor(transferQueue, submitSerial);
		}
		else
		{
			mQueue.WriteBuffer(mIndexBuffer, indices.data(), indices.size() * sizeof(uint32_t), 0);
			mQueue.WriteBuffer(mVertexBuffer, vertices.data(), vertices.size() * sizeof(Vertex), 0);
			mQueue.WriteBuffer(mUniformBuffer, &shaderData, sizeof(ShaderData), 0);
		}

	}

	void run()
	{
		while (!glfwWindowShouldClose(mWindow))
		{
			glfwPollEvents();

			mSurface.AcquireNextTexture();
			// draw 
			render();
			mSurface.Present();
			mDevice.Tick();
			mFrameCount++;
		}
	}

	void render()
	{

		{
			ColorAttachment colorAttachment{};
			colorAttachment.view = mSurface.GetCurrentTextureView();
			DepthStencilAattachment dsAttachment{};
			dsAttachment.view = mDsView;

			rhi::RenderPassDesc passDesc{};
			passDesc.colorAttachmentCount = 1;
			passDesc.colorAttachments = &colorAttachment;
			passDesc.depthStencilAttachment = &dsAttachment;
			RenderPassEncoder pass = mCommandEncoder.BeginRenderPass(passDesc);
			pass.SetPipeline(mPipeline);
			pass.SetBindSet(mBindSet, 0);
			pass.SetIndexBuffer(mIndexBuffer, IndexFormat::Uint32);
			pass.SetVertexBuffers(0, 1, &mVertexBuffer);
			pass.DrawIndexed(indices.size());
			pass.End();
		}
		CommandList commandList = mCommandEncoder.Finish();
		mQueue.Submit(&commandList, 1);
	}

	void cleanUp()
	{
		glfwTerminate();
	}
private:
	rhi::Buffer mUniformBuffer;
	rhi::Buffer mVertexBuffer;
	rhi::Buffer mIndexBuffer;
	rhi::RenderPipeline mPipeline;
	rhi::CommandEncoder mCommandEncoder;
	rhi::BindSetLayout mBindSetLayout;
	rhi::BindSet mBindSet;

	uint32_t mWindowWidth = 1024;
	uint32_t mWindowHeight = 768;
	GLFWwindow* mWindow;

	rhi::Texture mDepthStencilTexture;
	rhi::TextureView mDsView;

	rhi::Surface mSurface;
	rhi::Device mDevice;
	rhi::Queue mQueue;

	uint64_t mFrameCount = 0;

	static void GLFW_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (action == GLFW_PRESS)
		{
			if (key == GLFW_KEY_W)
			{
				camera.velocity.z = -1;
			}
			if (key == GLFW_KEY_S)
			{
				camera.velocity.z = 1;
			}
			if (key == GLFW_KEY_A)
			{
				camera.velocity.x = -1;
			}
			if (key == GLFW_KEY_D)
			{
				camera.velocity.x = 1;
			}
		}
		if (action == GLFW_RELEASE)
		{
			if (key == GLFW_KEY_W)
			{
				camera.velocity.z = 0;
			}
			if (key == GLFW_KEY_S)
			{
				camera.velocity.z = 0;
			}
			if (key == GLFW_KEY_A)
			{
				camera.velocity.x = 0;
			}
			if (key == GLFW_KEY_D)
			{
				camera.velocity.x = 0;
			}
		}

	}


	static void GLFW_CursorPosCallback(GLFWwindow* wnd, double xpos, double ypos)
	{
		float xscale = 1;
		float yscale = 1;
		glfwGetWindowContentScale(wnd, &xscale, &yscale);

		float xoffset = xpos - lastX;
		float yoffset = ypos - lastY; // reversed since y-coordinates go from bottom to top

		lastX = xpos;
		lastY = ypos;

		float mouseSensitivity = 0.005f;
		xoffset *= mouseSensitivity;
		yoffset *= mouseSensitivity;


		camera.yaw += xoffset;
		camera.pitch += yoffset;

		if (camera.pitch > 89.0f)
			camera.pitch = 89.0f;
		if (camera.pitch < -89.0f)
			camera.pitch = -89.0f;
	}
};


int main()
{
	App app;
	app.init();
	app.run();
	app.cleanUp();
}