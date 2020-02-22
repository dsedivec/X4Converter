#include "X4ConverterTools/model/Component.h"
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <utility>

using namespace boost::algorithm;
namespace model {
using util::XmlUtil;

Component::Component(ConversionContext::Ptr ctx) : AiNodeElement(std::move(ctx)) {}

Component::Component(pugi::xml_node &node, const ConversionContext::Ptr &ctx) : AiNodeElement(ctx) {
  auto componentNode = node;
  CheckXmlNode(node, "component");
  if (!componentNode.child("source")) {
    std::cerr << "source directory not specified" << std::endl;
  } else {
    std::string src = componentNode.child("source").attribute("geometry").value();
    if (src.empty()) {
      throw runtime_error("Source directory for geometry must be specified!");
    }
    attrs["source"] = src;
    ctx->SetSourcePathSuffix(src);
  }

  for (auto attr: componentNode.attributes()) {
    auto name = std::string(attr.name());
    auto value = std::string(attr.value());
    if (name == "name") {
      setName(value);
    } else {
      attrs[name] = value;
    }
  }
  auto connectionsNode = componentNode.child("connections");
  if (connectionsNode.empty()) {
    std::cerr << "Warning, could not find any <connection> nodes!" << std::endl;
  }
  for (auto connectionNode : connectionsNode.children()) {
    connections.emplace_back(connectionNode, ctx, getName());
  }

  auto layersNode = componentNode.child("layers");
  if (layersNode.empty()) {
    std::cerr << "Warning, <layers> node not found";
  }
  auto layerNode = layersNode.child("layer");
  // TODO handle these fully
  if (layerNode.next_sibling()) {
    std::cerr << "Warning, this file contains more than one layer. Ignoring all but the first." << std::endl;
    throw std::runtime_error("multiple layers");
  }

}

aiNode *Component::ConvertToAiNode() {
  auto result = new aiNode(getName());
  std::map<std::string, aiNode *> nodes;
  nodes[getName()] = result;
  ctx->AddMetadata(getName(), attrs);
  // Convert all the nodes
  for (auto conn : connections) {
    std::string connName = conn.getName();
    if (nodes.count(connName)) {
      throw std::runtime_error("Duplicate key is not allowed!" + connName);
    }
    nodes[connName] = conn.ConvertToAiNode();
    conn.ConvertParts(nodes);
  }
  AssimpUtil::printAiMap(nodes);
  // Now to unflatten everything
  std::map<std::string, std::vector<aiNode *>> parentMap;
  for (auto conn: connections) {
    auto parentName = conn.getParentName();
    if (!nodes.count(parentName)) {
      throw std::runtime_error("Missing parent \"" + parentName + "\" on: \"" + conn.getName() + "\"");
    }
//      std::cout << conn.getName() << " " << parentName << std::endl;
    auto connNode = nodes[conn.getName()];
    if (connNode == nullptr) {
      throw std::runtime_error("null ainode for connection " + conn.getName());
    }
    if (parentMap.count(parentName) == 0) {
      parentMap[parentName] = std::vector<aiNode *>();
    }
    parentMap[parentName].push_back(connNode);

  }
  for (const auto &pair : parentMap) {
    auto parentNode = nodes[pair.first];
    if (parentNode == nullptr) {
      throw std::runtime_error("null ainode for parent: " + pair.first);
    }
    populateAiNodeChildren(parentNode, pair.second);
  }

  // Now double check that we didn't miss anything
  for (auto conn: connections) {
    auto connNode = nodes[conn.getName()];
    if (connNode->mParent == nullptr) {
      throw std::runtime_error("connection" + conn.getName() + "lost its parent");
    }
  }
  for (const auto &pair : nodes) {
    if (pair.first == getName()) {
      continue;
    }
    if (pair.second->mParent == nullptr) {
      throw std::runtime_error(std::string(pair.second->mName.C_Str()) + "lost its parent");
    }
    for (int i = 0; i < pair.second->mNumChildren; i++) {
      if (pair.second->mChildren[i] == nullptr) {
        throw std::runtime_error(std::string(pair.second->mName.C_Str()) + "has null child");
      }
    }
  }
  auto rootChildren = new aiNode *[1];
  rootChildren[0] = result;
  auto fakeRoot = new aiNode("ROOT");
  fakeRoot->addChildren(1, rootChildren);
  delete[] rootChildren;
  return fakeRoot;
}

void Component::ConvertFromAiNode(aiNode *node) {
  setName(node->mName.C_Str());
  attrs = ctx->GetMetadataMap(getName());
  for (int i = 0; i < node->mNumChildren; i++) {
    auto child = node->mChildren[i];
    std::string childName = child->mName.C_Str();
    if (childName.find('*') == std::string::npos) {
      std::cerr << "Warning, possible non-component directly under root, ignoring: " << childName
                << std::endl;
    } else if (starts_with(childName, "layer")) {
      layers.emplace_back(child, ctx);

    } else {
      connections.emplace_back(child, ctx);
      recurseOnChildren(child, ctx);
    }
  }
}

void Component::recurseOnChildren(aiNode *tgt, const ConversionContext::Ptr &ctx) {
  std::string tgtName = tgt->mName.C_Str();
  bool is_connection = tgtName.find('*') != std::string::npos;
  for (int i = 0; i < tgt->mNumChildren; i++) {
    auto child = tgt->mChildren[i];
    std::string childName = child->mName.C_Str();
    if (childName.find('*') != std::string::npos) {
      if (is_connection) {
        throw std::runtime_error("connection cannot have a connection as a parent!");
      }
      connections.emplace_back(child, ctx, tgtName);
    }
    recurseOnChildren(child, ctx);
  }

}

void Component::ConvertToGameFormat(pugi::xml_node &out) {
  // TODO asset.xmf?
  if (std::string(out.name()) != "components") {
    throw std::runtime_error("Component should be under components element");
  }
  auto compNode = XmlUtil::AddChildByAttr(out, "component", "name", getName());
  auto connsNode = XmlUtil::AddChild(compNode, "connections");
  for (const auto &attr : attrs) {
    if (attr.first == "source") {
      XmlUtil::AddChildByAttr(compNode, "source", "geometry", attr.second);
      // TODO compare to output path and confirm if wrong
      ctx->SetSourcePathSuffix(attr.second);
    } else {
      XmlUtil::WriteAttr(compNode, attr.first, attr.second);
    }
  }
  for (auto conn : connections) {
    conn.ConvertToGameFormat(connsNode);
  }
}

uint32_t Component::getNumberOfConnections() {
  return boost::numeric_cast<uint32_t>(connections.size());
}
}