#include "pipeline_raster_opengl.h"

#include <glad/glad.h>

#include <spirv_glsl.hpp>

using namespace AnimationViewer::Graphics;

std::unique_ptr<Pipeline>
PipelineRasterOpenGL::create(const CreateInfo& info)
{
  int32_t is_compiled = 0;
  char compile_log[0x400];

  spirv_cross::CompilerGLSL::Options options;
  options.version = 300;
  options.es = true;

  spirv_cross::CompilerGLSL vertex_shader_compiler(info.vertex_shader_binary,
                                                   info.vertex_shader_size);
  vertex_shader_compiler.set_common_options(options);
  vertex_shader_compiler.set_entry_point(info.vertex_shader_entry_point, spv::ExecutionModelVertex);
  auto glsl_vertex_source = vertex_shader_compiler.compile();

  spirv_cross::CompilerGLSL fragment_shader_compiler(info.fragment_shader_binary,
                                                     info.fragment_shader_size);
  fragment_shader_compiler.set_common_options(options);
  fragment_shader_compiler.set_entry_point(info.fragment_shader_entry_point,
                                           spv::ExecutionModelFragment);
  auto glsl_fragment_source = fragment_shader_compiler.compile();

  uint32_t vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  auto glsl_vertex_source_c = glsl_vertex_source.c_str();
  glShaderSource(vertex_shader, 1, &glsl_vertex_source_c, nullptr);
  glCompileShader(vertex_shader);
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &is_compiled);
  if (!is_compiled) {
    glGetShaderInfoLog(vertex_shader, sizeof(compile_log), nullptr, compile_log);
    printf("Vertex Shader compilation failed: %s\n", compile_log);
    glDeleteShader(vertex_shader);
    return nullptr;
  }
  uint32_t fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  auto glsl_fragment_source_c = glsl_fragment_source.c_str();
  glShaderSource(fragment_shader, 1, &glsl_fragment_source_c, nullptr);
  glCompileShader(fragment_shader);
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &is_compiled);
  if (is_compiled == 0) {
    glGetShaderInfoLog(fragment_shader, sizeof(compile_log), nullptr, compile_log);
    printf("Fragment Shader compilation failed: %s\n", compile_log);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return nullptr;
  }

  uint32_t program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  glGetProgramiv(program, GL_LINK_STATUS, &is_compiled);
  if (!is_compiled) {
    glGetProgramInfoLog(program, sizeof(compile_log), nullptr, compile_log);
    printf("Program linking failed: %s\n", compile_log);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    glDeleteProgram(program);
    return nullptr;
  }

  return std::make_unique<PipelineRasterOpenGL>(program);
}

PipelineRasterOpenGL::PipelineRasterOpenGL(uint32_t program)
  : program_(program)
{}

PipelineRasterOpenGL::~PipelineRasterOpenGL()
{
  glDeleteProgram(program_);
}

void
PipelineRasterOpenGL::set_uniform(uint8_t location,
                                  Pipeline::UniformType type,
                                  uint32_t count,
                                  const void* value)
{
  glUseProgram(program_);
  float camera_fov_y = 0.173648178;
  auto location_ = glGetUniformLocation(program_, "camera_fov_y");
  if (location_ == -1) {
    printf("get location of camera_fov_y returned -1\n");
  }
  switch (type) {
    case Pipeline::UniformType::Float:
      glUniform1f(location_, camera_fov_y);
      break;
    case Pipeline::UniformType::Vec2:
      glUniform2fv(location, count, reinterpret_cast<const GLfloat*>(value));
      break;
  }
}

void
PipelineRasterOpenGL::bind()
{
  glFrontFace(GL_CW);
  glUseProgram(program_);
}

uint32_t
PipelineRasterOpenGL::get_native_handle() const
{
  return program_;
}
