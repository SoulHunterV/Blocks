#pragma once

#include <string>

#include <glm/glm.hpp>

#include "glew_headers.hpp"


class OpenglShader
{
public:
  OpenglShader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr);
  ~OpenglShader();

  void Use();
  void SetBool(const std::string& name, bool value) const;
  void SetInt(const std::string& name, int value) const;
  void SetFloat(const std::string& name, float value) const;

  void SetVec2(const std::string& name, const glm::vec2& value) const;
  void SetVec2(const std::string& name, float x, float y) const;
  void SetVec3(const std::string& name, const glm::vec3& value) const;
  void SetVec3(const std::string& name, float x, float y, float z) const;
  void SetVec4(const std::string& name, const glm::vec4& value) const;
  void SetVec4(const std::string& name, float x, float y, float z, float w) const;

  void SetIVec2(const std::string& name, int x, int y) const;

  void SetMat2(const std::string& name, const glm::mat2& mat) const;
  void SetMat3(const std::string& name, const glm::mat3& mat) const;
  void SetMat4(const std::string& name, const glm::mat4& mat) const;

private:
  unsigned int id_;

  void CheckCompileErrors(GLuint shader, std::string type);
};