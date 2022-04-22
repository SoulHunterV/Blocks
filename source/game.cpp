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

#include "compile_utils.hpp"
#include "resourceConfig.h"


std::shared_ptr<OpenglModel> CreateBlockModel();


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
  while (!window->IsWindowShouldClose())
  {
    glfwPollEvents();

    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime_ = currentFrame - lastFrame_;
    lastFrame_ = currentFrame;

    processInput(window);

    renderSystem.Clear();

    float framebufferRatio = (float)framebufferWidth_ / (float)framebufferHeight_;
    for (int i = 0; i < 5; i++)
    {
      glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3((float)i * 2, 0.0f, 0.0f));
      renderSystem.RenderModel(blockModel, model, camera_.get(), (float)framebufferWidth_ / (float)framebufferHeight_);
    }
    for (int i = 1; i < 3; i++)
    {
      glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, (float)i * 2, 0.0f));
      renderSystem.RenderModel(blockModel, model, camera_.get(), (float)framebufferWidth_ / (float)framebufferHeight_);
    }
    for (int i = 1; i < 2; i++)
    {
      glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, (float)i * 2));
      renderSystem.RenderModel(blockModel, model, camera_.get(), (float)framebufferWidth_ / (float)framebufferHeight_);
    }

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
