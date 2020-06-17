
#include <stdio.h>
#include <iostream>

#include <functional>

#include <memory>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_inverse.hpp"

#include "bvh.h"
#include "bvhloader.h"

#include "camera.h"
#include "shader.h"

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP

static GLuint vboVert, vboIndices;
static GLuint vao;

k::Bvh* bvh;
int frame = 0, frameChange = 1;

void update() {

  if (frameChange) {
    frame = frame + frameChange;
  } else {
    return;
  }

  int frameto = frame % bvh->getNumFrames();
  std::cout << "move to " << frameto << std::endl;
  bvh->moveTo(frameto);

  std::vector<glm::vec4> vertices;
  std::vector<GLuint> bvhindices;

  bvh->generateGeometry(vertices, bvhindices);

  glBindBuffer(GL_ARRAY_BUFFER, vboVert);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertices.size(), &vertices[0], GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
    exit(0);
  }
  if (key == GLFW_KEY_J && action == GLFW_PRESS) {
    frameChange++;
  }
  if (key == GLFW_KEY_K && action == GLFW_PRESS) {
    frameChange--;
  }
}

int main(int argc, char* argv[]) {
  START_EASYLOGGINGPP(argc, argv);
  LOG(INFO) << "Test SLOG INFO";

  GLFWwindow* window = nullptr;

  if (!glfwInit()) {
    LOG(ERROR) << "ERROR: could not start GLFW3";
    return 1;
  }

#if APPLE
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window = glfwCreateWindow(640, 480, "Hello world", nullptr, nullptr);

  if (!window) {
    LOG(ERROR) << "ERROR: could not open window with GLFW3";
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glewExperimental = GL_TRUE;
  glewInit();

  glfwSetKeyCallback(window, key_callback);

  const GLubyte* renderer;
  const GLubyte* version;
  /* get version info */
  renderer = glGetString(GL_RENDERER); /* get renderer string */
  version = glGetString(GL_VERSION);	 /* version as a string */
  LOG(INFO) << "Renderer: " << renderer;
  LOG(INFO) << "OpenGL version supported" << version;

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glLineWidth(3.0);
  glPointSize(5.0);

  int width = 512;
  int height = 512;

  k::Shader bvhShader;
  bvhShader.loadShader("data/bvh-loader/shaders/bvh-loader-shader");
  bvh = new k::Bvh;
  {
    k::BvhLoader bvhLoader;
    bvhLoader.load(bvh, "../data/bvh/0008_ChaCha001.bvh");
  }
  bvh->printJoint(bvh->getRootJoint());
  bvh->moveTo(frame++);

  // generate geometry
  std::vector<glm::vec4> vertices;
  std::vector<GLuint> bvhindices;

  bvh->generateGeometry(vertices, bvhindices);

  glGenBuffers(1, &vboVert);
  glBindBuffer(GL_ARRAY_BUFFER, vboVert);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertices.size(), &vertices[0], GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glGenBuffers(1, &vboIndices);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(bvhindices[0]) * bvhindices.size(), &bvhindices[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  {
    bvhShader.use();

    GLint sLocPosition = bvhShader.attribute("vPosition");

    glBindBuffer(GL_ARRAY_BUFFER, vboVert);
    glEnableVertexAttribArray(sLocPosition);
    glVertexAttribPointer(sLocPosition, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    bvhShader.unuse();
  }
  glBindVertexArray(0);

  k::Camera camera(width, height);
  camera.move(glm::vec3(-28.13f, 30.11f, 20.11f));

  while (!glfwWindowShouldClose(window)) {
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 mvp = camera.matrix();

    bvhShader.use();

    glm::vec3 bvh_scale = glm::vec3(0.25, 0.25, 0.25);
    glm::mat4 _model = glm::scale(glm::mat4(1.0), bvh_scale);

    mvp = camera.matrix() * _model;
    glUniformMatrix4fv(bvhShader.uniform("MVP"), 1, GL_FALSE, glm::value_ptr(mvp));

    update();
    glBindVertexArray(vao);
    // we must bind this
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
    glPointSize(5.0);
    glLineWidth(3.0);
    glDrawElements(GL_LINES, bvhindices.size(), GL_UNSIGNED_INT, (GLvoid*) 0);
    glDrawElements(GL_POINTS, bvhindices.size(), GL_UNSIGNED_INT, (GLvoid*) 0);
    glBindVertexArray(0);
    bvhShader.unuse();

    glfwSwapBuffers( window );
    glfwPollEvents();
  }

  return 0;
}
