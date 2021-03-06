#include "opengl_chunk.hpp"


namespace blocks
{
  OpenglChunk::OpenglChunk(std::shared_ptr<OpenglBuffer> vbo, std::shared_ptr<OpenglVertexArrayObject> vao, int verticesNumber)
    : vbo_(vbo), vao_(vao), verticesNumber_(verticesNumber)
  {

  }

  OpenglChunk::~OpenglChunk()
  {

  }
}
