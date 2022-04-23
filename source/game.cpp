#include "game.hpp"

#include <memory>
#include <string>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "render/opengl_model.hpp"
#include "render/opengl_chunk_model.hpp"
#include "model/chunk.hpp"

#include "compile_utils.hpp"
#include "resourceConfig.h"


std::shared_ptr<OpenglModel> CreateBlockModel();
std::shared_ptr<Chunk> GenerateChunk();
std::shared_ptr<OpenglChunkModel> GenerateChunkModel(std::shared_ptr<Chunk> chunk);


Game::Game(int width, int height) : framebufferWidth_(width), framebufferHeight_(height), lastX_(width / 2), lastY_(height / 2)
{
  platform_ = std::make_unique<GlfwPlatform>();
  camera_ = std::make_unique<Camera>(glm::vec3(0.0f, 0.0f, 3.0f));

  platform_->Init();
}

Game::~Game()
{
  platform_->Deinit();
}


int Game::Run()
{
  std::shared_ptr<GlfwWindow> window = platform_->CreateWindow(framebufferWidth_, framebufferHeight_, "Title");
  window->MakeCurrentContext();

  OpenglRenderSystem renderSystem;
  renderSystem.Init();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;

  ImGui::StyleColorsDark();

  window->InitImgui();
  // GL 3.0 + GLSL 130
  const char* glsl_version = "#version 130";
  ImGui_ImplOpenGL3_Init(glsl_version);

  // Set callbacks
  window->SetFramebufferCallback(
    [this](int width, int height)
    {
      framebufferWidth_ = width;
      framebufferHeight_ = height;

      glViewport(0, 0, width, height);
    }
  );
  window->SetKeyCallback(
    [window](int keycode, int scancode, int action, int mods) 
    {
      if (action != GLFW_PRESS || window == nullptr)
        return;
  
      if (keycode == GLFW_KEY_ESCAPE)
      {
        window->SetWindowShouldClose(true);
      }
    }
  );
  window->SetCursorPositionCallback(
    [this](double xpos, double ypos)
    {
      float xposF = static_cast<float>(xpos);
      float yposF = static_cast<float>(ypos);

      if (firstMouse_)
      {
        lastX_ = xposF;
        lastY_ = yposF;
        firstMouse_ = false;
      }

      float xoffset = xposF - lastX_;
      float yoffset = lastY_ - yposF; // reversed since y-coordinates go from bottom to top

      lastX_ = xposF;
      lastY_ = yposF;

      camera_->ProcessMouseMovement(xoffset, yoffset);
    }
  );
  window->SetScrollCallback(
    [this](double xoffset, double yoffset)
    {
      camera_->ProcessMouseScroll(static_cast<float>(yoffset));
    }
  );

  std::shared_ptr<OpenglModel> blockModel = CreateBlockModel();

  std::shared_ptr<Chunk> chunk = GenerateChunk();
  std::shared_ptr<OpenglChunkModel> chunkModel = GenerateChunkModel(chunk);
  std::shared_ptr<OpenglTexture> texture = std::make_shared<OpenglTexture>(PPCAT(TEXTURES_DIR, BLOCK_TEXTURE));
  texture->Bind(GL_TEXTURE0);

  while (!window->IsWindowShouldClose())
  {
    glfwPollEvents();

    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime_ = currentFrame - lastFrame_;
    lastFrame_ = currentFrame;

    processInput(window);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Info");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    renderSystem.Clear();

    float framebufferRatio = (float)framebufferWidth_ / (float)framebufferHeight_;

    renderSystem.RenderChunk(chunkModel, camera_.get(), framebufferRatio);

    // Render blockModel in different positions
    //for (int i = 0; i < 5; i++)
    //{
    //  glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3((float)i * 2, 0.0f, 0.0f));
    //  renderSystem.RenderModel(blockModel, model, camera_.get(), (float)framebufferWidth_ / (float)framebufferHeight_);
    //}
    //for (int i = 1; i < 3; i++)
    //{
    //  glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, (float)i * 2, 0.0f));
    //  renderSystem.RenderModel(blockModel, model, camera_.get(), (float)framebufferWidth_ / (float)framebufferHeight_);
    //}
    //for (int i = 1; i < 2; i++)
    //{
    //  glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, (float)i * 2));
    //  renderSystem.RenderModel(blockModel, model, camera_.get(), (float)framebufferWidth_ / (float)framebufferHeight_);
    //}

    // Render imgui ui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    window->SwapBuffers();
  }

  renderSystem.Deinit();

  return 0;
}

void Game::processInput(std::shared_ptr<GlfwWindow> window)
{
  if (window->GetKeyState(GLFW_KEY_W) == GLFW_PRESS)
  {
    camera_->ProcessKeyboard(Camera::FORWARD, deltaTime_);
  }
  if (window->GetKeyState(GLFW_KEY_S) == GLFW_PRESS)
  {
    camera_->ProcessKeyboard(Camera::BACKWARD, deltaTime_);
  }
  if (window->GetKeyState(GLFW_KEY_A) == GLFW_PRESS)
  {
    camera_->ProcessKeyboard(Camera::LEFT, deltaTime_);
  }
  if (window->GetKeyState(GLFW_KEY_D) == GLFW_PRESS)
  {
    camera_->ProcessKeyboard(Camera::RIGHT, deltaTime_);
  }
}


std::shared_ptr<OpenglModel> CreateBlockModel()
{
  std::shared_ptr<OpenglBuffer> vbo = std::make_shared<OpenglBuffer>(GL_ARRAY_BUFFER);
  std::shared_ptr<OpenglVertexArrayObject> vao = std::make_shared<OpenglVertexArrayObject>();
  std::shared_ptr<OpenglTexture> texture = std::make_shared<OpenglTexture>(PPCAT(TEXTURES_DIR, BLOCK_TEXTURE));

  float vertices[] = {
      -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
       0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
       0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
       0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
      -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
      -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

      -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
       0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
       0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
       0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
      -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
      -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

      -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
      -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
      -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
      -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
      -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
      -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

       0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
       0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
       0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
       0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
       0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
       0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

      -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
       0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
       0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
       0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
      -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
      -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

      -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
       0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
       0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
       0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
      -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
      -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
  };

  vao->Bind();

  vbo->Bind();
  vbo->SetData(sizeof(vertices), vertices);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  return std::make_shared<OpenglModel>(vbo, vao, texture);
}

std::shared_ptr<Chunk> GenerateChunk()
{
  Chunk* chunk = new Chunk();

  for (int z = 0; z < Chunk::Height; z++)
  {
    for (int y = 0; y < Chunk::Width; y++)
    {
      for (int x = 0; x < Chunk::Length; x++)
      {
        chunk->blocks[x + y * Chunk::Width + z * Chunk::LayerBlocksNumber] = (rand() % 4) > 0;
      }
    }
  }

  return std::shared_ptr<Chunk>(chunk);
}

struct Vertex
{
  float x;
  float y;
  float z;
  float texU;
  float texV;
};

void AddVertex(Vertex& vertex, float* data, size_t& index)
{
  data[index++] = vertex.x;
  data[index++] = vertex.y;
  data[index++] = vertex.z;
  data[index++] = vertex.texU;
  data[index++] = vertex.texV;
}

std::shared_ptr<OpenglChunkModel> GenerateChunkModel(std::shared_ptr<Chunk> chunk)
{
  std::shared_ptr<OpenglBuffer> vbo = std::make_shared<OpenglBuffer>(GL_ARRAY_BUFFER);
  std::shared_ptr<OpenglVertexArrayObject> vao = std::make_shared<OpenglVertexArrayObject>();
  std::shared_ptr<OpenglTexture> texture = std::make_shared<OpenglTexture>(PPCAT(TEXTURES_DIR, BLOCK_TEXTURE));

  static const size_t BlockVerticesNumber = 4 * 6;
  static const size_t VertexSize = sizeof(float) * 5;
  static const size_t verticesDataSize = Chunk::BlocksNumber * BlockVerticesNumber * VertexSize;

  float* verticesData = new float[verticesDataSize];
  size_t verticesDataIndex = 0;
  size_t tringlesNumber = 0;
  for (int z = 0; z < Chunk::Height; z++)
  {
    for (int y = 0; y < Chunk::Width; y++)
    {
      for (int x = 0; x < Chunk::Length; x++)
      {
        size_t blockIndex = x + y * Chunk::Width + z * Chunk::LayerBlocksNumber;

        if (chunk->blocks[blockIndex] == 0)
        {
          continue;
        }

        glm::vec3 position(x, y, z);

        // Check forward face
        if (x == Chunk::Length - 1 || chunk->blocks[blockIndex + 1] == 0)
        {
          // Add forward face

          Vertex v1(x + 1, y + 1, z, 0.0f, 0.0f);
          Vertex v2(x + 1, y, z, 1.0f, 0.0f);
          Vertex v3(x + 1, y + 1, z + 1, 0.0f, 1.0f);
          Vertex v4(x + 1, y, z + 1, 1.0f, 1.0f);

          AddVertex(v1, verticesData, verticesDataIndex);
          AddVertex(v2, verticesData, verticesDataIndex);
          AddVertex(v3, verticesData, verticesDataIndex);
          AddVertex(v3, verticesData, verticesDataIndex);
          AddVertex(v2, verticesData, verticesDataIndex);
          AddVertex(v4, verticesData, verticesDataIndex);

          tringlesNumber += 6;
        }

        // Check backward face
        if (x == 0 || chunk->blocks[blockIndex - 1] == 0)
        {
          // Add backward face

          Vertex v1(x, y, z, 0.0f, 0.0f);
          Vertex v2(x, y + 1, z, 1.0f, 0.0f);
          Vertex v3(x, y, z + 1, 0.0f, 1.0f);
          Vertex v4(x, y + 1, z + 1, 1.0f, 1.0f);

          AddVertex(v1, verticesData, verticesDataIndex);
          AddVertex(v2, verticesData, verticesDataIndex);
          AddVertex(v3, verticesData, verticesDataIndex);
          AddVertex(v3, verticesData, verticesDataIndex);
          AddVertex(v2, verticesData, verticesDataIndex);
          AddVertex(v4, verticesData, verticesDataIndex);

          tringlesNumber += 6;
        }

        // Check right face
        if (y == Chunk::Width - 1 || chunk->blocks[blockIndex + Chunk::Length] == 0)
        {
          // Add right face

          Vertex v1(x, y + 1, z, 0.0f, 0.0f);
          Vertex v2(x + 1, y + 1, z, 1.0f, 0.0f);
          Vertex v3(x, y + 1, z + 1, 0.0f, 1.0f);
          Vertex v4(x + 1, y + 1, z + 1, 1.0f, 1.0f);

          AddVertex(v1, verticesData, verticesDataIndex);
          AddVertex(v2, verticesData, verticesDataIndex);
          AddVertex(v3, verticesData, verticesDataIndex);
          AddVertex(v3, verticesData, verticesDataIndex);
          AddVertex(v2, verticesData, verticesDataIndex);
          AddVertex(v4, verticesData, verticesDataIndex);

          tringlesNumber += 6;
        }

        // Check left face
        if (y == 0 || chunk->blocks[blockIndex - Chunk::Length] == 0)
        {
          // Add left face

          Vertex v1(x + 1, y, z, 0.0f, 0.0f);
          Vertex v2(x, y, z, 1.0f, 0.0f);
          Vertex v3(x + 1, y, z + 1, 0.0f, 1.0f);
          Vertex v4(x, y, z + 1, 1.0f, 1.0f);

          AddVertex(v1, verticesData, verticesDataIndex);
          AddVertex(v2, verticesData, verticesDataIndex);
          AddVertex(v3, verticesData, verticesDataIndex);
          AddVertex(v3, verticesData, verticesDataIndex);
          AddVertex(v2, verticesData, verticesDataIndex);
          AddVertex(v4, verticesData, verticesDataIndex);

          tringlesNumber += 6;
        }

        // Check upper face
        if (z == Chunk::Height - 1 || chunk->blocks[blockIndex + Chunk::LayerBlocksNumber] == 0)
        {
          // Add upper face

          Vertex v1(x + 1, y, z + 1, 0.0f, 0.0f);
          Vertex v2(x, y, z + 1, 1.0f, 0.0f);
          Vertex v3(x + 1, y + 1, z + 1, 0.0f, 1.0f);
          Vertex v4(x, y + 1, z + 1, 1.0f, 1.0f);

          AddVertex(v1, verticesData, verticesDataIndex);
          AddVertex(v2, verticesData, verticesDataIndex);
          AddVertex(v3, verticesData, verticesDataIndex);
          AddVertex(v3, verticesData, verticesDataIndex);
          AddVertex(v2, verticesData, verticesDataIndex);
          AddVertex(v4, verticesData, verticesDataIndex);

          tringlesNumber += 6;
        }

        // Check bottom face
        if (z == 0 || chunk->blocks[blockIndex - Chunk::LayerBlocksNumber] == 0)
        {
          // Add bottom face

          Vertex v1(x, y, z, 0.0f, 0.0f);
          Vertex v2(x + 1, y, z, 1.0f, 0.0f);
          Vertex v3(x, y + 1, z, 0.0f, 1.0f);
          Vertex v4(x + 1, y + 1, z, 1.0f, 1.0f);

          AddVertex(v1, verticesData, verticesDataIndex);
          AddVertex(v2, verticesData, verticesDataIndex);
          AddVertex(v3, verticesData, verticesDataIndex);
          AddVertex(v3, verticesData, verticesDataIndex);
          AddVertex(v2, verticesData, verticesDataIndex);
          AddVertex(v4, verticesData, verticesDataIndex);

          tringlesNumber += 6;
        }
      }
    }
  }

  vao->Bind();

  vbo->Bind();
  vbo->SetData(sizeof(float) * (verticesDataIndex + 1), verticesData);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  return std::make_shared<OpenglChunkModel>(vbo, vao, tringlesNumber);
}
