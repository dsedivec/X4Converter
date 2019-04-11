#pragma once

#include <pugixml.hpp>
#include <map>
#include <stdexcept>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>

#include <assimp/IOSystem.hpp>
#include <assimp/ProgressHandler.hpp>
#include <assimp/scene.h>
#include <assimp/SceneCombiner.h>
#include <X4ConverterTools/util/PathUtil.h>
#include <X4ConverterTools/Part.h>
#include <X4ConverterTools/Lod.h>

class Component {
public:
    static std::shared_ptr<Component>
    ReadFromFile(const std::string &filePath, const ConversionContext &context, Assimp::IOSystem *pIOHandler);


    // Import Conversion
    aiNode *ConvertToAiNode(ConversionContext &context);

    // Export
    void WriteToFile(const std::string &filePath, const std::string &gameBaseFolderPath, Assimp::IOSystem *pIOHandler);

    std::string Name;
    std::map<std::string, Part> Parts;

private:
    static pugi::xml_node GetComponentNode(pugi::xml_document &doc);

    static boost::filesystem::path
    GetGeometryFolderPath(pugi::xml_node componentNode, const std::string &gameBaseFolderPath, bool createIfMissing);

    void CreateDummyFile(const std::string &filePath, const std::string &gameBaseFolderPath);


};
