#ifndef BVH_H
#define BVH_H

#include <vector>
#include <string>

#include "glm/glm.hpp"

namespace k {

typedef struct {
    float x, y, z;
} OFFSET;

typedef struct JOINT JOINT;

struct JOINT {
    // joint name
    std::string name;

    // joint parent
    JOINT* parent = nullptr;

    // offset data
    OFFSET offset;

    // num of channels joint has
    unsigned int num_channels = 0;

    // ordered list of channels
    short* channels_order = nullptr;

    // joint's children
    std::vector<JOINT*> children;

    // local transofrmation matrix (premultiplied with parents'
    glm::mat4 matrix;

    // index of joint's channel data in motion array
    unsigned int channel_start = 0;
};

struct HIERARCHY {
    JOINT* rootJoint;
    int num_channels;
};

struct MOTION {
    unsigned int num_frames;
    unsigned int num_motion_channels = 0;
    float* data = nullptr;
    unsigned* joint_channel_offsets;
};

struct bvh_statistics {
    unsigned int num_hierarchies;
    unsigned int num_total_joints;
    unsigned int num_motion_channels = 0;
    unsigned int num_frames;

};

/**
 * @brief The Bvh class
 */
class Bvh {
public:
    Bvh();
    ~Bvh();

    void printJoint(const JOINT* const joint);
    void moveTo(unsigned frame);
    void generateGeometry(std::vector<glm::vec4>& outVertices, std::vector<uint32_t>& outIndicies);

    const JOINT* getRootJoint() const { return rootJoint; }
    unsigned getNumFrames() const { return motionData.num_frames; }
private:
    JOINT* rootJoint;

    MOTION motionData;

    friend class BvhLoader;
};

}

#endif
