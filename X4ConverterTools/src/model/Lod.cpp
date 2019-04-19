#include "X4ConverterTools/model/Lod.h"

// TODO fold in or split off lod
model::Lod::Lod(pugi::xml_node node, std::string parentName) {

    if (std::string(node.name()) != "lod") {
        throw std::runtime_error("XML element must be a <lod> element!");
    }
    if (node.attribute("index").empty()) {
        throw std::runtime_error("Lod must have an index attribute!");
    }
    index = node.attribute("index").as_int();
    setName(str(boost::format("%1%^lod%2%") % parentName % index));
}

aiNode *model::Lod::ConvertToAiNode() {
    return new aiNode(name);
}

void model::Lod::ConvertFromAiNode(aiNode *node) {
    std::string rawName = node->mName.C_Str();
    setName(rawName);
    // Parse out the index
    size_t pos = rawName.rfind("^lod");
    if (pos == std::string::npos) {
        throw std::runtime_error("lod lacks index");
    }
    index = std::stoi(rawName.substr(pos + 4));

}

int model::Lod::getIndex() {
    return index;
}