#pragma once

#include "pugixml.hpp"
#include "zlib.h"
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <assimp/scene.h>
#include <X4ConverterTools/util/PathUtil.h>

class Material {
public:
    Material();

    Material(std::string pCollectionName, pugi::xml_node node);

    const std::string GetCollectionName() { return _pCollectionName; }

    const std::string &GetName() { return _name; }


    aiMaterial *
    ConvertToAiMaterial(const boost::filesystem::path &modelFolderPath, const boost::filesystem::path &baseFolderPath);

private:
    const std::string GetTextureFilePath (std::string filePath, const boost::filesystem::path &baseFolderPath) const;

    const
    std::string
    GetDecompressedTextureFilePath(const std::string filePath, const boost::filesystem::path &baseFolderPath) const;

    void populateDiffuseLayer (aiMaterial *pAiMaterial, const boost::filesystem::path &modelFolderPat, const boost::filesystem::path &baseFolderPat);

    void populateSpecularLayer(aiMaterial *pAiMaterial, const boost::filesystem::path &modelFolderPath, const boost::filesystem::path &baseFolderPath);

    void populateNormalLayer  (aiMaterial *pAiMaterial, const boost::filesystem::path &modelFolderPath, const boost::filesystem::path &baseFolderPat);

    void populateEnvironmentLayer (aiMaterial *pAiMaterial, const boost::filesystem::path &modelFolderPath, const boost::filesystem::path &baseFolderPath);

    std::string _pCollectionName;
    std::string _name;

    aiColor4D _emissiveColor;

    std::string _diffuseMapFilePath;
    std::string _specularMapFilePath;
    std::string _normalMapFilePath;
    std::string _environmentMapFilePath;

    float _diffuseStrength;
    float _normalStrength;
    float _specularStrength;
    float _environmentStrength;
};
