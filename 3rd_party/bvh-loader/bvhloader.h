#ifndef BVHLOADER_H
#define BVHLOADER_H

#include <string>
#include <iostream>

#include "bvh.h"

namespace k {

class BvhLoader {
  void loadHierarchy(Bvh* bvh, std::istream& stream);
  void loadMotion(Bvh* bvh, std::istream& stream);
  JOINT* loadJoint(Bvh* bvh, std::istream& stream, JOINT* parent = nullptr);
public:
  BvhLoader();
  void load(Bvh* bvh, const std::string& filename);
};

}

#endif // BVHLOADER_H
