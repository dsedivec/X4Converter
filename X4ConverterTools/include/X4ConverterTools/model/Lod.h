#pragma once

#include <X4ConverterTools/xmf/XmfFile.h>
#include <boost/algorithm/string.hpp>
#include <utility>
#include "AbstractElement.h"

namespace model {
class Lod : public AbstractElement {
 public:
  static constexpr char Qualifier[] = "[Lo]";
  explicit Lod(ConversionContext::Ptr ctx) : AbstractElement(std::move(ctx), Qualifier) {}

  aiNode *ConvertToAiNode() override {
    if (!ctx->ShouldConvertGeometry()) {
      throw std::runtime_error("Should not be converting in this mode.");
    }
    return xmfFile->ConvertToAiNode(getQualifiedName());
  }

  void CalculateSizeAndCenter(aiNode *pNode) {
    auto numMeshes = pNode->mNumMeshes;
    if (numMeshes != 1) {
      std::cerr << "Warning, node " << getName() << " has " << numMeshes << " meshes. Should have one." << std::endl;
      return;
//    throw std::runtime_error(ss.str());
    }

    aiMesh *pMesh = ctx->GetMesh(pNode->mMeshes[0]);
    auto numVertices = pMesh->mNumVertices;
    if (numVertices == 0) {
      std::stringstream ss;
      ss << "Warning, node " << getName() << " has an 0 vertex mesh." << std::endl;
      throw std::runtime_error(ss.str());
    }

    aiVector3D lowerBound = pMesh->mVertices[0];
    aiVector3D upperBound = pMesh->mVertices[0];
    for (size_t i = 1; i < numVertices; ++i) {
      aiVector3D &position = pMesh->mVertices[i];
      lowerBound.x = std::min(lowerBound.x, position.x);
      lowerBound.y = std::min(lowerBound.y, position.y);
      lowerBound.z = std::min(lowerBound.z, position.z);

      upperBound.x = std::max(upperBound.x, position.x);
      upperBound.y = std::max(upperBound.y, position.y);
      upperBound.z = std::max(upperBound.z, position.z);
    }

    // TODO flip axes if necessary
    maxDim = (upperBound - lowerBound) / 2.0f;
    center = (upperBound + lowerBound) / 2.0f;
    center.x = -center.x;
  }
  ~Lod() override = default;

  int getIndex() {
    return index;
  }

 protected:
  std::shared_ptr<xmf::XmfFile> xmfFile;
  int index = -2;
  aiVector3D maxDim;
  aiVector3D center;
};

}