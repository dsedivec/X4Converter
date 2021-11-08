#include <X4ConverterTools/ani/AnimFile.h>


// Callee should free

using namespace boost;
using namespace Assimp;
using boost::numeric_cast;
namespace ani {
AnimFile::AnimFile() : header() {

}

AnimFile::AnimFile(Assimp::StreamReaderLE &pStreamReader) {
  header = Header(pStreamReader);
  descs = std::vector<AnimDesc>();
  for (int i = 0; i < header.getNumAnims(); i++) {
    descs.emplace_back(pStreamReader);
  }
  if (pStreamReader.GetCurrentPos() != header.getKeyOffsetBytes()) {
    std::string err = str(format("AnimFile: current position (%1%) does not align with the data offset (%2%)") %
        pStreamReader.GetCurrentPos() % header.getKeyOffsetBytes());
    throw std::runtime_error(err);
  }
  for (int i = 0; i < header.getNumAnims(); i++) {
    descs[i].read_frames(pStreamReader);
  }
  validate();
  // TODO fixme
}

AnimFile::AnimFile(pugi::xml_node &node) {
  header = Header();
  pugi::xml_node dataNode = node.child("data");
  pugi::xml_node metaNode = node.child("metadata");
  for (auto &part : dataNode.children()) {
    std::string partName = part.attribute("name").as_string();
    for (auto &animCat : part.children()) {
      for (auto &anim : animCat.children()) {
        // TODO move this one layer down and assert only 1 category
        auto subName = anim.attribute("subname").value();
        // TODO some throw statements here
        auto partMeta = metaNode.find_child_by_attribute("connection", "name", partName.c_str());
        auto animMeta = partMeta.find_child_by_attribute("animation", "subname", subName);
        descs.emplace_back(partName, anim, animMeta);
      }
    }
  }
  header.setNumAnims(numeric_cast<int>(descs.size()));
}

AnimFile::~AnimFile() = default;

Header AnimFile::GetHeader() const {
  return header;
}


std::string AnimFile::validate() {
  std::string s;
  s.append(header.validate());
  for (auto desc : descs) {
    try {
      std::string ret = desc.validate();
      s.append(ret);
    } catch (std::exception &e) {
      s.append(e.what());
      throw std::runtime_error(s);
    }
  }
  if (descs.size() != header.getNumAnims()) {
    s.append(str(format("Expected %1% descriptions but only read %2%") % descs.size() % header.getNumAnims()));
  }

  return s;
}

void AnimFile::WriteIntermediateRepr(const std::string &xmlPath, pugi::xml_node &tgtNode) const {

  // TODO integrate into Component?
  // TODO validate non-overlap
  // TODO please refactor

  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_file(xmlPath.c_str());
  if (result.status != pugi::status_ok) {
    throw std::runtime_error(str(format("Failed to parse %s: %s") % xmlPath % result.description()));
  }
  pugi::xml_node componentNode = doc.select_node("/components/component").node();
  if (!componentNode) {
    throw std::runtime_error("File has no <component> element");
  }

  pugi::xpath_node_set connections = componentNode.select_nodes("connections/connection");
  for (auto conn : connections) {
    // TODO what if multiple parts?
    auto connNode = conn.node();
    HandleConnection(tgtNode, connNode);

  }
  std::cout << "Writing animations!" << std::endl;
  pugi::xml_node dataNode = tgtNode.append_child("data");
  for (const AnimDesc &desc : descs) {
    desc.WriteIntermediateRepr(dataNode);
  }

}

void AnimFile::HandleConnection(pugi::xml_node &tgtNode, pugi::xml_node &conn) const {
  pugi::xml_node partNode = conn.child("parts").child("part");
  std::string name = partNode.attribute("name").value();

  pugi::xpath_node_set animMetas = conn.select_nodes("animations/animation");
  if (animMetas.empty()) {
    return;
  }
  pugi::xml_node outMetaNode = tgtNode.child("metadata");
  if (!outMetaNode) {
    outMetaNode = tgtNode.append_child("metadata");
  }
  pugi::xml_node outConnNode = outMetaNode.append_child("connection");
  outConnNode.append_attribute("name").set_value(name.c_str());

  for (auto animEntry : animMetas) {
    auto animMeta = animEntry.node();
    // TODO what about the hidden ones?
    // TODO deal with parts vs components
    pugi::xml_node outAnimNode = outConnNode.append_child("animation");
    outAnimNode.append_attribute("subname").set_value(animMeta.attribute("name").as_string());
    pugi::xml_node outFrameNode = outAnimNode.append_child("frames");
    outFrameNode.append_attribute("start").set_value(animMeta.attribute("start").as_int());
    outFrameNode.append_attribute("end").set_value(animMeta.attribute("end").as_int());
  }
  // TODO cleaner way?
  if (!partNode.child("pivot").empty() && !partNode.child("pivot").child("offset").empty() &&
      !partNode.child("pivot").child("offset").child("position").empty()) {
    std::cout << "Wrote pivot " << std::endl;
    auto outPivotNode = outConnNode.append_child("pivot_position_offset");
    for (auto attr : partNode.child("pivot").child("offset").child("position").attributes()) {
      outPivotNode.append_attribute(attr.name()).set_value(attr.value());
    }
  }

}

void AnimFile::WriteGameFiles(Assimp::StreamWriterLE &writer, pugi::xml_node &node) {
  auto numAnims = descs.size();
  header.setNumAnims(numAnims);

  // Write header
  header.WriteGameFiles(writer);
  // Write animation descriptions
  for (int i = 0; i < numAnims; i++) {
    descs[i].WriteToGameFiles(writer);
  }
  if (writer.GetCurrentPos() != header.getKeyOffsetBytes()) {
    std::string err = str(format("AnimFile: current position (%1%) does not align with the data offset (%2%)") %
        writer.GetCurrentPos() % header.getKeyOffsetBytes());
    throw std::runtime_error(err);
  }
  // Write corresponding keyframes
  for (int i = 0; i < numAnims; i++) {
    descs[i].write_frames(writer);
  }
}

}