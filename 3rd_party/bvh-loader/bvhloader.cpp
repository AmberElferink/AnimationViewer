#include "bvhloader.h"

#include <iostream>
#include <string>
#include <fstream>
#include <set>
#include <sstream>
#include <algorithm>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "util.h"

#define Xposition 0x01
#define Yposition 0x02
#define Zposition 0x04
#define Zrotation 0x10
#define Xrotation 0x20
#define Yrotation 0x40

namespace k {

BvhLoader::BvhLoader() {

}

void BvhLoader::load(Bvh* bvh, const std::string& filename) {
  std::fstream file;
  file.open(filename.c_str(), std::ios_base::in);

  if (file.is_open()) {
    std::string line;

    while(file.good()) {
      file >> line;
      if (trim(line) == "HIERARCHY") {
        loadHierarchy(bvh, file);
      }
      break;
    }
    file.close();
  }
}

void BvhLoader::loadHierarchy(Bvh* bvh, std::istream& stream) {
  std::string tmp;

  while (stream.good()) {
    stream >> tmp;

    if( trim(tmp) == "ROOT" ) {
      bvh->rootJoint = loadJoint(bvh, stream);
    } else if (trim(tmp) == "MOTION") {
      loadMotion(bvh, stream);
    }
  }
}

void BvhLoader::loadMotion(Bvh* bvh, std::istream& stream) {
  std::string tmp;

  MOTION& motionData = bvh->motionData;

  while (stream.good()) {
    stream >> tmp;

    if (trim(tmp) == "Frames:") {
      stream >> motionData.num_frames;
    } else if (trim(tmp) == "Frame") {
      float frame_time;
      stream >> tmp >> frame_time;
      motionData.frame_time = frame_time;

      int num_frames   = motionData.num_frames;
      int num_channels = motionData.num_motion_channels;

      motionData.data = new float[num_frames * num_channels];

      for (int frame = 0; frame < num_frames; frame++) {
        for (int channel = 0; channel < num_channels; channel++) {
          float x;
          std::stringstream ss;
          stream >> tmp;
          ss << tmp;
          ss >> x;

          int index = frame * num_channels + channel;
          motionData.data[index] = x;
        }
      }
    }
  }
}

JOINT* BvhLoader::loadJoint(Bvh* bvh, std::istream& stream, JOINT* parent) {
  MOTION& motionData = bvh->motionData;
  JOINT* joint = new JOINT;
  joint->parent = parent;
  joint->matrix = glm::mat4(1.0);

  // load joint name
  stream >> joint->name;

  std::string tmp;
  joint->matrix = glm::mat4(1.0);

  static int _channel_start = 0;
  unsigned channel_order_index = 0;
  while (stream.good()) {
    stream >> tmp;
    tmp = trim(tmp);

    // setting channels
    char c = tmp.at(0);
    if (c == 'X' || c == 'Y' || c == 'Z') {
      if (tmp == "Xposition") {
        joint->channels_order[channel_order_index++] = Xposition;
      }
      if (tmp == "Yposition") {
        joint->channels_order[channel_order_index++] = Yposition;
      }
      if (tmp == "Zposition") {
        joint->channels_order[channel_order_index++] = Zposition;
      }

      if (tmp == "Xrotation") {
        joint->channels_order[channel_order_index++] = Xrotation;
      }
      if (tmp == "Yrotation") {
        joint->channels_order[channel_order_index++] = Yrotation;
      }
      if (tmp == "Zrotation") {
        joint->channels_order[channel_order_index++] = Zrotation;
      }
    }

    if (tmp == "OFFSET") {
      stream  >> joint->offset.x
          >> joint->offset.y
          >> joint->offset.z;
    } else if (tmp == "CHANNELS") {
      // loading num of channels
      stream >> joint->num_channels;

      // adding to statistics
      motionData.num_motion_channels += joint->num_channels;

      // increasing static counter of channel index startin motion section
      joint->channel_start = _channel_start;
      _channel_start += joint->num_channels;

      // creating an array for channel order specification
      joint->channels_order = new short[joint->num_channels];

    } else if (tmp == "JOINT") {
      JOINT* tmp_joint = loadJoint(bvh, stream, joint);

      tmp_joint->parent = joint;
      joint->children.push_back(tmp_joint);

    } else if (tmp == "End") {
      // End Site {
      stream >> tmp >> tmp;

      JOINT* tmp_joint = new JOINT;

      tmp_joint->parent = joint;
      tmp_joint->num_channels = 0;
      tmp_joint->name = "EndSite";
      joint->children.push_back(tmp_joint);

      stream >> tmp;
      if (tmp == "OFFSET") {
        stream >> tmp_joint->offset.x
            >> tmp_joint->offset.y
            >> tmp_joint->offset.z;
      }

      // reading '}'
      stream >> tmp;
    } else if (tmp == "}") {
      return joint;
    }
  }
  return joint;
}

}
