#include "bvh.h"

#include <iostream>
#include <string>
#include <fstream>
#include <set>
#include <sstream>
#include <algorithm>

#include "glm/gtc/matrix_transform.hpp"

#define Xposition 0x01
#define Yposition 0x02
#define Zposition 0x04
#define Zrotation 0x10
#define Xrotation 0x20
#define Yrotation 0x40

namespace k {

Bvh::Bvh()
  : rootJoint(NULL)
{
  motionData.data = 0;
}

void deleteJoint(JOINT* joint) {
  if (joint == nullptr) {
    return;
  }

  for (JOINT* child : joint->children) {
    deleteJoint(child);
  }

  if (joint->channels_order != nullptr) {
    delete[] joint->channels_order;
  }

  delete joint;
}

Bvh::~Bvh() {
  deleteJoint(rootJoint);

  if (motionData.data != nullptr) {
    delete[] motionData.data;
  }
}

void Bvh::printJoint(const JOINT* const joint) {
  for (int i = 0; i < joint->children.size(); i++) {
    JOINT* _tmp = joint->children.at(i);

    if (_tmp->children.size() > 0) {
      printJoint(_tmp);
    }
  }
}

void internalGenerateGeometry(JOINT* joint,
                              std::vector<glm::vec4>& outVertices,
                              std::vector<uint32_t>& outIndicies,
                              int parentIndex = 0)
{
  glm::vec4 translatedVertex = joint->matrix[3];

  outVertices.push_back(translatedVertex);

  uint16_t myindex = outVertices.size() - 1;

  if (parentIndex != myindex) {
    outIndicies.push_back(parentIndex);
    outIndicies.push_back(myindex);
  }

  for(auto& child : joint->children) {
    internalGenerateGeometry(child, outVertices, outIndicies, myindex);
  }
}

void Bvh::generateGeometry(std::vector<glm::vec4>& outVertices,
                           std::vector<uint32_t>& outIndicies) {
  internalGenerateGeometry(rootJoint, outVertices, outIndicies);
}

// calculate local transformation matrix for specific frame - revisit this
void moveJoint(JOINT* joint, MOTION* motionData, int frame_starts_index) {
  int start_index = frame_starts_index + joint->channel_start;

  // postavljamo prvo lokalnu mat na offset poziciju
  joint->matrix = glm::translate(glm::mat4(1.0),
                                 glm::vec3(joint->offset.x,
                                           joint->offset.y,
                                           joint->offset.z));

  for(int i = 0; i < joint->num_channels; i++) {
    const short& channel = joint->channels_order[i];

    float value = motionData->data[start_index + i];
    if (channel & Xposition) {
      joint->matrix = glm::translate(joint->matrix, glm::vec3(value, 0, 0));
    }
    if (channel & Yposition) {
      joint->matrix = glm::translate(joint->matrix, glm::vec3(0, value, 0));
    }
    if (channel & Zposition) {
      joint->matrix = glm::translate(joint->matrix, glm::vec3(0, 0, value));
    }

    if (channel & Xrotation) {
      joint->matrix = glm::rotate(joint->matrix, glm::radians(value), glm::vec3(1, 0, 0));
    }
    if (channel & Yrotation) {
      joint->matrix = glm::rotate(joint->matrix, glm::radians(value), glm::vec3(0, 1, 0));
    }
    if (channel & Zrotation) {
      joint->matrix = glm::rotate(joint->matrix, glm::radians(value), glm::vec3(0, 0, 1));
    }
  }

  // mul current matrix with one of a parent
  if (joint->parent != NULL) {
    joint->matrix = joint->parent->matrix * joint->matrix;
  }

  for (auto& child : joint->children) {
    moveJoint(child, motionData, frame_starts_index);
  }
}

void Bvh::moveTo(unsigned frame) {
  // calculate start index for specific frame for motion data
  unsigned start_index = frame * motionData.num_motion_channels;

  // skeleton transformation
  moveJoint(rootJoint, &motionData, start_index);
}

}
