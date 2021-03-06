#include "game.hpp"

#include <memory>
#include <thread>
#include <string>
#include <iostream>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <format>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "io/file_api.hpp"
#include "ui/i_imgui_element.hpp"
#include "ui/imgui_button.hpp"
#include "ui/imgui_text.hpp"
#include "ui/imgui_window.hpp"


namespace blocks
{
  Game::Game(int width, int height) : 
    window_(Environment::GetPlatform().CreateWindow(width, height, "Blocks Game"))
  {
    context_.scene = CreateMainMenuScene();
    context_.camera = std::make_shared<Camera>(glm::vec3(8.0f, 8.0f, 270.0f));
    context_.playerBounds = blocks::AABB(glm::vec3(-0.25f, -0.25f, -0.25f), glm::vec3(0.25f, 0.25f, 0.25f));

    inputModule_.SetupWindow(window_);
  }

  Game::~Game()
  {
    // Nothing to do here
  }

  int Game::Run()
  {
    std::thread renderThread(&Game::RunRenderCycle, this);
    std::thread renderUpdateThread(&Game::RunRenderUpdateCycle, this);

    RunSimulationCycle();

    renderThread.join();
    renderUpdateThread.join();

    return 0;
  }


  void Game::RunSimulationCycle()
  {
    std::condition_variable cv;
    std::mutex mut;

    using delta = std::chrono::duration<std::int64_t, std::ratio<1, 60>>;
    auto next = std::chrono::steady_clock::now() + delta{ 1 };
    std::unique_lock<std::mutex> lk(mut);

    const static float deltaF = 1.0f / 60.0f;
    long counter = 0;
    while (isRunning_)
    {
      if (window_.IsWindowShouldClose())
      {
        isRunning_ = false;
        continue;
      }

      mut.unlock();

      {
        if (requestedScene_ != nullptr)
        {
          SetRequestedScene();
        }

        inputModule_.Update();
        InputState& inputState = inputModule_.GetState();

        if (inputState.IsKeyJustPressed(GLFW_KEY_ESCAPE))
        {
          window_.SetWindowShouldClose(true);
          isRunning_ = false;
        }

        if (inputState.IsKeyJustPressed(GLFW_KEY_L))
        {
          SwitchCursorMode(window_);
        }

        playerControlModule_.Update(deltaF, inputState, context_);
        mapLoadingModule_.Update(deltaF, context_);
      }

      mut.lock();
      cv.wait_until(lk, next, [] {return false; });
      next += delta{ 1 };
    }
  }

  void Game::RunRenderCycle()
  {
    GlfwPlatform& platform = Environment::GetPlatform();

    window_.SetCursorMode(CursorMode::Normal);
    window_.SetCurrentContext();

    renderModule_.SetContext(window_);
    renderModule_.InitResources();
    context_.openglScene = renderModule_.GetOpenglScene();

    mapLoadingModule_.SetRenderModule(&renderModule_);

    float lastTime = (float)platform.GetTime();
    while (isRunning_)
    {
      float currentTime = (float)platform.GetTime();
      float delta = currentTime - lastTime;
      lastTime = currentTime;

      sceneMutex_.lock();
      renderModule_.Update(delta, context_);
      sceneMutex_.unlock();

      window_.SwapBuffers();
    }

    renderModule_.FreeResources();
  }

  void Game::RunRenderUpdateCycle()
  {
    std::condition_variable cv;
    std::mutex mut;

    using delta = std::chrono::duration<std::int64_t, std::ratio<1, 16>>;
    auto next = std::chrono::steady_clock::now() + delta{ 1 };
    std::unique_lock<std::mutex> lk(mut);

    const static float deltaF = 1.0f / 16.0f;
    long counter = 0;
    while (isRunning_)
    {
      mut.unlock();

      {
        mapLoadingModule_.ProcessChunksToAdd(deltaF, context_);
      }

      mut.lock();
      cv.wait_until(lk, next, [] {return false; });
      next += delta{ 1 };
    }
  }


  void Game::SwitchCursorMode(GlfwWindow& window)
  {
    context_.isCursorEnabled = !context_.isCursorEnabled;

    if (context_.isCursorEnabled)
    {
      window.SetCursorMode(CursorMode::Normal);
    }
    else
    {
      window.SetCursorMode(CursorMode::Disabled);
    }
  }


  void Game::RequestScene(std::shared_ptr<Scene> scene)
  {
    requestedScene_ = scene;
  }

  void Game::SetRequestedScene()
  {
    sceneMutex_.lock();

    context_.scene = requestedScene_;
    requestedScene_ = nullptr;

    mapLoadingModule_.OnSceneChanged(context_);

    sceneMutex_.unlock();
  }

  std::shared_ptr<Scene> Game::CreateMainMenuScene()
  {
    std::shared_ptr<Scene> scene = std::make_shared<Scene>();

    std::shared_ptr<ImguiWindow> window = std::make_shared<ImguiWindow>("Main menu");
    scene->AddImguiWindow(window);

    std::shared_ptr<ImguiButton> createWorldButton = std::make_shared<ImguiButton>(
      "Create new world",
      [this]()
      {
        srand(time(0));
        std::shared_ptr<Scene> worldScene_ = CreateWorldScene(std::make_shared<Map>(rand()));
        RequestScene(worldScene_);
      }
    );
    window->AddElement(createWorldButton);

    std::shared_ptr<ImguiButton> loadWorldButton = std::make_shared<ImguiButton>(
      "Load world",
      [this]()
      {
        if (!blocks::isPathExist("map"))
        {
          return;
        }

        std::shared_ptr<Map> map = Map::Load();
        std::shared_ptr<Scene> worldScene_ = CreateWorldScene(map);

        RequestScene(worldScene_);
      }
    );
    window->AddElement(loadWorldButton);

    return scene;
  }

  std::shared_ptr<Scene> Game::CreateWorldScene(std::shared_ptr<Map> map)
  {
    std::shared_ptr<Scene> scene = std::make_shared<Scene>();

    scene->SetMap(map);

    std::shared_ptr<ImguiWindow> window = std::make_shared<ImguiWindow>("Statistics");
    scene->AddImguiWindow(window);

    std::shared_ptr<ImguiText> fpsText = std::make_shared<ImguiText>(
      [this]()
      {
        return std::format("Application average {0:.3f} ms/frame ({1:.0f} FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      }
    );
    window->AddElement(fpsText);

    std::shared_ptr<ImguiText> cameraPositionText = std::make_shared<ImguiText>(
      [this]()
      {
        glm::vec3 position = context_.camera->GetPosition();
        return std::format("Camera position: {0:.2f} {1:.2f} {2:.2f}", position.x, position.y, position.z);
      }
    );
    window->AddElement(cameraPositionText);

    std::shared_ptr<ImguiText> cameraDirectionText = std::make_shared<ImguiText>(
      [this]()
      {
        glm::vec3 direction = context_.camera->GetForward();
        return  std::format("Camera direction: {0:.2f} {1:.2f} {2:.2f}", direction.x, direction.y, direction.z);
      }
    );
    window->AddElement(cameraDirectionText);

    std::shared_ptr<ImguiText> seedText = std::make_shared<ImguiText>(
      [this]()
      {
        return  std::format("Map seed: {}", context_.scene->GetMap()->GetSeed());
      }
    );
    window->AddElement(seedText);

    std::shared_ptr<ImguiButton> saveButton = std::make_shared<ImguiButton>(
      "Save world",
      [this]()
      {
        if (!blocks::isPathExist("map"))
        {
          blocks::createDirectory("map");
        }
        else
        {
          for (const std::string& path : blocks::getFilesInDirectory("map"))
          {
            blocks::removePath(path);
          }
        }

        Map::Save(context_.scene->GetMap());
      }
    );
    window->AddElement(saveButton);

    return scene;
  }
}
