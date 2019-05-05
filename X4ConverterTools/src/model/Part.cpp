#include "X4ConverterTools/model/Part.h"

#include <string>
#include <iostream>
#include <X4ConverterTools/model/CollisionLod.h>
#include <X4ConverterTools/model/VisualLod.h>
#include <regex>

namespace model {
    Part::Part(std::shared_ptr<ConversionContext> ctx) : AbstractElement(ctx) {
        hasRef = false;
        collisionLod = nullptr;
    }

    Part::Part(pugi::xml_node node, std::shared_ptr<ConversionContext> ctx) : AbstractElement(ctx) {
        if (std::string(node.name()) != "part") {
            throw std::runtime_error("XML element must be a <part> element!");
        }
        if (node.attribute("name").empty()) {
            throw std::runtime_error("Part must have a name attribute!");
        }
        hasRef = false;
        for (auto attr: node.attributes()) {
            auto attrName = std::string(attr.name());
            if (attrName == "ref") {
                hasRef = true;
                attrs["DO_NOT_EDIT.ref"] = attr.value();
            } else if (attrName == "name") {
                setName(attr.value());
            } else {
                std::cerr << "Warning, unhandled attribute on part: " << getName() << " attribute: " << attrName
                          << ". This may work fine, just a heads up ;)" << std::endl;
                attrs[attrName] = attr.value();
            }
        }

        auto lodsNode = node.child("lods");
        if (hasRef && !lodsNode.empty()) {
            throw std::runtime_error("ref should not contain lods");
        }

        // TODO figure out a better way
        if (!hasRef) {
            collisionLod = std::make_unique<CollisionLod>(getName(), ctx);

            for (auto lodNode : lodsNode.children()) {
                auto lod = VisualLod(lodNode, getName(), ctx);
                lods.insert(std::pair<int, VisualLod>(lod.getIndex(), lod));
            }
        }

    }


    aiNode *Part::ConvertToAiNode(std::shared_ptr<ConversionContext> ctx) {
        auto *result = new aiNode(getName());
        std::vector<aiNode *> children = attrToAiNode();
        if (!hasRef) {
            children.push_back(collisionLod->ConvertToAiNode(ctx));
            for (auto lod: lods) {
                children.push_back(lod.second.ConvertToAiNode(ctx));
            }
        }

        populateAiNodeChildren(result, children);


        return result;
    }

    static std::regex lodRegex("[^|]+\\|lod\\d");
    static std::regex collisionRegex("[^|]+\\|collision");

    void Part::ConvertFromAiNode(aiNode *node, std::shared_ptr<ConversionContext> ctx) {
        std::string tmp = std::string();
        setName(node->mName.C_Str());
        for (int i = 0; i < node->mNumChildren; i++) {
            auto child = node->mChildren[i];
            std::string childName = child->mName.C_Str();
            // TODO check part names?
            if (regex_match(childName, lodRegex)) {
                auto lod = VisualLod(ctx);
                lod.ConvertFromAiNode(child, ctx);
                lods.insert(std::pair<int, VisualLod>(lod.getIndex(), lod));
            } else if (regex_match(childName, collisionRegex)) {
                collisionLod = std::make_unique<CollisionLod>(ctx);
                collisionLod->ConvertFromAiNode(child, ctx);
            } else if (childName.find('*') != std::string::npos) {
                // Ignore connection, handled elsewhere
            } else {
                readAiNodeChild(child);
            }
        }
        // TODO more
    }

    void Part::ConvertToGameFormat(pugi::xml_node out, std::shared_ptr<ConversionContext> ctx) {
        if (std::string(out.name()) != "parts") {
            throw std::runtime_error("part must be appended to a parts xml element");
        }

        auto partNode = ChildByAttr(out, "part", "name", getName());


        // Note the return statement! referenced parts don't get LODS!!!
        // TODO remove if lods exist or at least error out
        if (attrs.count("DO_NOT_EDIT.ref")) {
            hasRef = true;
            auto value = attrs["DO_NOT_EDIT.ref"];
            if (partNode.attribute("ref")) {
                partNode.attribute("ref").set_value(value.c_str());
            } else {
                partNode.prepend_attribute("ref").set_value(value.c_str());
            }
            return;
        }
        for (const auto &attr : attrs) {
            WriteAttr(partNode, attr.first, attr.second);
        }

        if (!lods.empty()) {
            auto lodsNode = Child(partNode, "lods");
            collisionLod->ConvertToGameFormat(lodsNode, ctx); // TODO
            for (auto lod : lods) {
                lod.second.ConvertToGameFormat(lodsNode, ctx);
            }
        } else {
            partNode.remove_child("lods");
        }
        // TODO out more
    }
}