#pragma once
#include <X4ConverterTools/Material.h>
#include <X4ConverterTools/MaterialCollection.h>
#include <regex>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/range.hpp>
#include <boost/filesystem.hpp>
#include <assimp/scene.h>
#include <X4ConverterTools/Types.h>
#include "pugixml.hpp"
class MaterialLibrary
{
    friend Material;

public:
                                                    MaterialLibrary     ( const std::string& gameBaseFolderPath );

    MaterialCollection*                             GetCollection       ( const std::string& name );
    Material*                                       GetMaterial         ( const std::string& dottedName );

private:
    std::string                                     _gameBaseFolderPath;
    pugi::xml_document                              _doc;
    std::map < std::string, MaterialCollection >    _collections;
};
