#include "map.hpp"

#include "FastNoise/FastNoise.h"

#include "io/file_api.hpp"


namespace blocks
{
  Map::Map()
  {
    seed_ = rand();
  }

  Map::Map(int seed) : seed_(seed)
  {

  }

  Map::~Map()
  {

  }


  int Map::GetSeed()
  {
    return seed_;
  }

  std::shared_ptr<Chunk> Map::GetChunk(std::pair<int, int> position)
  {
    std::lock_guard<std::mutex> locker(mutex_);

    std::shared_ptr<Chunk> chunk;

    auto it = chunks_.find(position);
    if (it == chunks_.end())
    {
      chunk = GenerateChunk(position);
      chunks_[position] = chunk;
    }
    else
    {
      chunk = it->second;
    }

    return chunk;
  }

  std::pair<std::map<std::pair<int, int>, std::shared_ptr<Chunk>>::iterator, std::map<std::pair<int, int>, std::shared_ptr<Chunk>>::iterator> Map::GetChunksIterator()
  {
    return std::make_pair(chunks_.begin(), chunks_.end());
  }

  void Map::AddChunk(std::pair<int, int> position, std::shared_ptr<Chunk> chunk)
  {
    std::lock_guard<std::mutex> locker(mutex_);

    chunks_[position] = chunk;
  }


  bool Map::Collides(const blocks::AABB& bounds, glm::vec3 position)
  {
    std::pair<int, int> chunkPosition = std::make_pair(position.x / Chunk::Length, position.y / Chunk::Width);
    glm::vec3 localPosition = glm::vec3(position.x - chunkPosition.first * (int)Chunk::Length, position.y - chunkPosition.second * (int)Chunk::Width, position.z);
    if (localPosition.x < 0)
    {
      localPosition.x += Chunk::Length;
      chunkPosition.first--;
    }
    if (localPosition.y < 0)
    {
      localPosition.y += Chunk::Width;
      chunkPosition.second--;
    }
    std::shared_ptr<Chunk> chunk = GetChunk(chunkPosition);

    blocks::AABB localBounds(bounds.low + localPosition, bounds.high + localPosition);

    glm::ivec3 centralBlockPosition = glm::ivec3(localPosition);

    int radius = 2;
    for (int x = centralBlockPosition.x - radius; x <= centralBlockPosition.x + radius; x++)
    {
      if (x < 0 || x >= Chunk::Length)
      {
        continue;
      }

      for (int y = centralBlockPosition.y - radius; y <= centralBlockPosition.y + radius; y++)
      {
        if (y < 0 || y >= Chunk::Width)
        {
          continue;
        }

        for (int z = centralBlockPosition.z - radius; z <= centralBlockPosition.z + radius; z++)
        {
          if (z < 0 || z >= Chunk::Height)
          {
            continue;
          }

          if (chunk->blocks[x + y * Chunk::Width + z * Chunk::LayerBlocksNumber] == 0)
          {
            continue;
          }

          blocks::AABB blockBounds(glm::vec3(x, y, z), glm::vec3(x + 1, y + 1, z + 1));
          if (CheckCollision(blockBounds, localBounds))
          {
            return true;
          }
        }
      }
    }

    return false;
  }

  BlockLookAt Map::GetBlockLookAt(const blocks::Ray& ray)
  {
    std::pair<int, int> chunkPosition = std::make_pair(ray.origin.x / Chunk::Length, ray.origin.y / Chunk::Width);
    glm::vec3 localPosition = glm::vec3(ray.origin.x - chunkPosition.first * (int)Chunk::Length, ray.origin.y - chunkPosition.second * (int)Chunk::Width, ray.origin.z);
    if (localPosition.x < 0)
    {
      localPosition.x += Chunk::Length;
      chunkPosition.first--;
    }
    if (localPosition.y < 0)
    {
      localPosition.y += Chunk::Width;
      chunkPosition.second--;
    }

    std::shared_ptr<Chunk> chunk = GetChunk(chunkPosition);
    blocks::Ray localRay(localPosition, ray.direction);

    glm::ivec3 centralBlockPosition = glm::ivec3(localPosition);

    blocks::RayIntersectionPoint closestIntersectionPoint;
    glm::ivec3 intersectedBlock;
    int radius = 3;
    for (int x = centralBlockPosition.x - radius; x <= centralBlockPosition.x + radius; x++)
    {
      if (x < 0 || x >= Chunk::Length)
      {
        continue;
      }

      for (int y = centralBlockPosition.y - radius; y <= centralBlockPosition.y + radius; y++)
      {
        if (y < 0 || y >= Chunk::Width)
        {
          continue;
        }

        for (int z = centralBlockPosition.z - radius; z <= centralBlockPosition.z + radius; z++)
        {
          if (z < 0 || z >= Chunk::Height)
          {
            continue;
          }

          if (chunk->blocks[x + y * Chunk::Width + z * Chunk::LayerBlocksNumber] == 0)
          {
            continue;
          }

          blocks::AABB blockBounds(glm::vec3(x, y, z), glm::vec3(x + 1, y + 1, z + 1));
          blocks::RayIntersectionPoint intersectionPoint = CheckCollision(localRay, blockBounds);
          if (intersectionPoint.distance != FLT_MAX && closestIntersectionPoint.distance > intersectionPoint.distance)
          {
            closestIntersectionPoint = intersectionPoint;
            intersectedBlock = glm::ivec3(x, y, z);
          }
        }
      }
    }

    BlockLookAt result;
    result.chunkPosition = chunkPosition;
    result.blockPosition = intersectedBlock;
    if (closestIntersectionPoint.distance != FLT_MAX)
    {
      result.hit = true;
      if (closestIntersectionPoint.point.x == (float)intersectedBlock.x)
      {
        result.loockFromDirection = Direction::Back;
      }
      else if (closestIntersectionPoint.point.x == (float)(intersectedBlock.x + 1))
      {
        result.loockFromDirection = Direction::Forward;
      }
      else if (closestIntersectionPoint.point.y == (float)intersectedBlock.y)
      {
        result.loockFromDirection = Direction::Left;
      }
      else if (closestIntersectionPoint.point.y == (float)(intersectedBlock.y + 1))
      {
        result.loockFromDirection = Direction::Right;
      }
      else if (closestIntersectionPoint.point.z == (float)intersectedBlock.z)
      {
        result.loockFromDirection = Direction::Down;
      }
      else if (closestIntersectionPoint.point.z == (float)(intersectedBlock.z + 1))
      {
        result.loockFromDirection = Direction::Up;
      }
    }

    return result;
  }


  std::shared_ptr<Map> Map::Load()
  {
    std::string seedStr = blocks::readTextFile("map/seed.txt");
    int seed = std::stoi(seedStr);

    std::shared_ptr<Map> map = std::make_shared<Map>(seed);

    for (const std::string& path : blocks::getFilesInDirectory("map"))
    {
      if (path == "seed.txt")
      {
        continue;
      }

      std::vector<unsigned char> data = blocks::readBinaryFile("map/" + path);
      std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>();
      memcpy(chunk.get(), &data[0], sizeof(Chunk));

      size_t underscorePosition = path.find("_");
      size_t dotPosition = path.find(".");

      std::string xStr = path.substr(0, underscorePosition);
      int x = std::stoi(xStr);
      std::string yStr = path.substr(underscorePosition + 1, dotPosition - underscorePosition - 1);
      int y = std::stoi(yStr);
      std::pair<int, int> position = std::make_pair(x, y);

      map->AddChunk(position, chunk);
    }

    return map;
  }

  void Map::Save(std::shared_ptr<Map> map)
  {
    blocks::saveTextFile("map/seed.txt", std::to_string(map->GetSeed()));

    auto chunksIterator = map->GetChunksIterator();
    for (auto it = chunksIterator.first; it != chunksIterator.second; it++)
    {
      std::string path = std::format("map/{0}_{1}.chunk", it->first.first, it->first.second);

      size_t a = sizeof(Chunk);
      std::vector<unsigned char> data(sizeof(Chunk));
      memcpy(&data[0], it->second.get(), sizeof(Chunk));

      blocks::saveBinaryFile(path, data);
    }
  }


  std::shared_ptr<Chunk> Map::GenerateChunk(std::pair<int, int> position)
  {
    Chunk* chunk = new Chunk();

    float* highMap = new float[Chunk::LayerBlocksNumber];

    auto perlinNoise = FastNoise::New<FastNoise::Perlin>();
    auto minMax = perlinNoise->GenUniformGrid2D(highMap, position.first * Chunk::Length, position.second * Chunk::Width, Chunk::Length, Chunk::Width, 0.01f, seed_);

    Block blockType = (rand() % 4) + 1;
    for (int x = 0; x < Chunk::Length; x++)
    {
      for (int y = 0; y < Chunk::Width; y++)
      {
        float height = (highMap[x + y * Chunk::Length] + 1.0f) / 2.0f; // (0.0 - 1.0) range
        int highBorder = (int)(height * Chunk::Height);

        for (int z = 0; z < highBorder; z++)
        {
          chunk->blocks[x + y * Chunk::Length + z * Chunk::LayerBlocksNumber] = blockType;
        }
      }
    }

    return std::shared_ptr<Chunk>(chunk);
  }
}
