#pragma once

#include <pugixml.hpp>
#include <map>
#include <stdexcept>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>

#include <assimp/IOSystem.hpp>
#include <X4ConverterTools/util/PathUtil.h>
#include <X4ConverterTools/Lod.h>

class Part {
public:
    Part() = default;

    explicit Part(pugi::xml_node partNode, const boost::filesystem::path &geometryFolderPath,
                           Assimp::IOSystem *pIOHandler);

    Lod *GetLod(int lodIndex);

    std::string Name;
    std::string ParentName;
    std::vector<Lod> Lods;
    std::shared_ptr<xmf::XuMeshFile> CollisionMesh;
    aiVector3D Position;
    aiVector3D Center;
    aiVector3D Size;
    aiVector3D Offset;

    aiQuaterniont<ai_real> Rot;

    // Import Conversion
    aiNode *ConvertToAiNode(ConversionContext &context);



    // For Exporting back to the game:
    void WritePart(pugi::xml_node connectionsNode, const boost::filesystem::path &geometryFolderPath,
                   Assimp::IOSystem *pIOHandler);


    void PrepareLodNodeForExport(int lodIndex, const aiScene *pScene, aiNode *pLodNode);
    void PrepareCollisionNodeForExport(const aiScene *pScene, aiNode *pCollisionNode);
protected:
    void WritePartParent(pugi::xml_node partNode);

    void WritePartPosition(pugi::xml_node partNode);

    void WritePartSize(pugi::xml_node partNode);

    void WritePartCenter(pugi::xml_node partNode);

    void WritePartLods(pugi::xml_node partNode, const boost::filesystem::path &geometryFolderPath,
                       Assimp::IOSystem *pIOHandler);


};