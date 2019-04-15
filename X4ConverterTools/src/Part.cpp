#include <X4ConverterTools/Part.h>

using namespace boost;
using namespace xmf;

Lod *Part::GetLod(int lodIndex) {
    for (Lod &lod: Lods) {
        if (lod.Index == lodIndex) {
            return &lod;
        }
    }
    return nullptr;
}

Part::Part(pugi::xml_node partNode, const boost::filesystem::path &geometryFolderPath,
                             Assimp::IOSystem *pIOHandler) {

    Name = partNode.attribute("name").value();

    pugi::xml_attribute parentAttr = partNode.parent().parent().attribute("parent");
    if (parentAttr)
        ParentName = parentAttr.value();

    pugi::xml_node posNode = partNode.select_node("../../offset/position").node();
    if (posNode) {
        Position = aiVector3D(-posNode.attribute("x").as_float(), posNode.attribute("y").as_float(),
                              posNode.attribute("z").as_float());
    }

    pugi::xml_node sizeNode = partNode.select_node("size/max").node();
    if (sizeNode) {
        Size = aiVector3D(sizeNode.attribute("x").as_float(), sizeNode.attribute("y").as_float(),
                          sizeNode.attribute("z").as_float());
    }

    // TODO handle rotation & offset here too if possible
    pugi::xml_node rotNode = partNode.select_node("../../offset/quaternion").node();
    if (rotNode) {
        Rot = aiQuaternion(rotNode.attribute("qw").as_float(), rotNode.attribute("qx").as_float(),
                           rotNode.attribute("qy").as_float(), rotNode.attribute("qz").as_float());
        std::cout << "rotation..." << std::endl;
    }

    // TODO are there any pivot rotations?
    pugi::xml_node pivotNode = partNode.select_node("pivot").node();
    if (!pivotNode.empty() && !pivotNode.child("offset").empty()) {
        std::cout << "Found pivot" << std::endl;
        pugi::xml_node offsetNode = pivotNode.child("offset");
        if (!offsetNode.child("position").empty()) {
            pugi::xml_node posOffsetNode = offsetNode.child("position");
            Offset = aiVector3D(-posOffsetNode.attribute("x").as_float(),posOffsetNode.attribute("y").as_float(),
            posOffsetNode.attribute("z").as_float());

        } else {
            Offset = aiVector3D(0,0,0);
            std::cerr << "Warning: pivot & offset found, but no position node!\n";
        }
    } else {
        Offset = aiVector3D(0,0,0);
        std::cerr << "Warning: pivot found, but no offset node!\n";
    }

    // TODO check for extra child nodes of pivot/offset


    int lodIndex = 0;
    while (true) {
        // TODO what about the mysterious assets .xmfs?
        //TODO better solution for path generation & debugging
        boost::filesystem::path lodFilePath = geometryFolderPath / (format("%s-lod%d.xmf") % Name % lodIndex).str();
        std::cout << "reading lod .xmf: " << lodFilePath << std::endl;
        if (!is_regular_file(lodFilePath)) {
            break;
        }

        std::shared_ptr<XuMeshFile> pMeshFile = XuMeshFile::ReadFromFile(lodFilePath.string(), pIOHandler);
        Lods.emplace_back(lodIndex, pMeshFile);
        lodIndex++;
    }

    //TODO better solution
    boost::filesystem::path collisionFilePath = geometryFolderPath / (Name + "-collision.xmf");
    std::cout << "reading collison .xmf: " << collisionFilePath << std::endl;
    if (is_regular_file(collisionFilePath)) {
        CollisionMesh = XuMeshFile::ReadFromFile(collisionFilePath.string(), pIOHandler);
    }
}


// Import conversion
aiNode *Part::ConvertToAiNode(ConversionContext &context) {
    auto *pPartNode = new aiNode();
    try {
        pPartNode->mName = Name;

        // TODO push this into part
        auto outRot  = Rot;// * aiQuaternion(0,-M_PI,0);
        // -X because handedness I guess. But we have to do the same when applying other data
        (pPartNode->mTransformation) = aiMatrix4x4(aiVector3D(1, 1, 1), outRot,Position);

        pPartNode->mChildren = new aiNode *[Lods.size() + (CollisionMesh ? 1 : 0)];

        for (Lod &lod : Lods) {
            const std::string name = (format("%sXlod%d") % Name % lod.Index).str();
            auto child = lod.Mesh->ConvertToAiNode(name, context);
            aiMatrix4x4::Translation(Offset,child->mTransformation);
            pPartNode->mChildren[pPartNode->mNumChildren++] = child;
        }
        if (CollisionMesh) {
            std::string name = Name + "Xcollision";

            auto child = CollisionMesh->ConvertToAiNode(name, context);
            aiMatrix4x4::Translation(Offset,child->mTransformation);
            pPartNode->mChildren[pPartNode->mNumChildren++] = child;
        }
    } catch (...) {
        // TODO real exception handling
        delete pPartNode;
        throw;
    }
    return pPartNode;
}


void Part::PrepareLodNodeForExport(int lodIndex, const aiScene *pScene, aiNode *pLodNode) {
    for (Lod &lod: Lods) {
        if (lod.Index == lodIndex)
            throw std::runtime_error((format("Duplicate lod index %d for part %s") % lodIndex % Name).str());
    }

    Lods.emplace_back(lodIndex, XuMeshFile::GenerateMeshFile(pScene, pLodNode, false));
}

void Part::PrepareCollisionNodeForExport(const aiScene *pScene, aiNode *pCollisionNode) {
    CollisionMesh = XuMeshFile::GenerateMeshFile(pScene, pCollisionNode, true);

    if (pCollisionNode->mNumMeshes == 0){
        std::cerr << "Empty collision mesh!\n";
        return;
    }
    // TODO this goes in the XMF too?
    aiVector3D lowerBound;
    aiVector3D upperBound;
    // TODO can we have more than 1 mesh or is that bad?
    aiMesh *pCollisionMesh = pScene->mMeshes[pCollisionNode->mMeshes[0]];
    for (int i = 0; i < pCollisionMesh->mNumVertices; ++i) {
        aiVector3D &position = pCollisionMesh->mVertices[i];
        if (i == 0) {
            lowerBound = position;
            upperBound = position;
        } else {
            lowerBound.x = std::min(lowerBound.x, position.x);
            lowerBound.y = std::min(lowerBound.y, position.y);
            lowerBound.z = std::min(lowerBound.z, position.z);

            upperBound.x = std::max(upperBound.x, position.x);
            upperBound.y = std::max(upperBound.y, position.y);
            upperBound.z = std::max(upperBound.z, position.z);
        }
    }

    Size = aiVector3D((upperBound.x - lowerBound.x) / 2.0f, (upperBound.y - lowerBound.y) / 2.0f,
                           (upperBound.z - lowerBound.z) / 2.0f);
    Center = aiVector3D(lowerBound.x + Size.x, lowerBound.y +Size.y, lowerBound.z + Size.z);
}

void Part::WritePart(pugi::xml_node connectionsNode, const boost::filesystem::path &geometryFolderPath,
                              Assimp::IOSystem *pIOHandler) {
    pugi::xml_node connectionNode;
    pugi::xml_node partNode = connectionsNode.select_node(
            (format("connection/parts/part[@name='%s']") % Name).str().c_str()).node();
    if (!partNode) {
        connectionNode = connectionsNode.append_child("connection");
        size_t numConnections = connectionsNode.select_nodes("connection").size();
        // Note that this appends the connection (e.g. Connection(current-index)
        connectionNode.append_attribute("name").set_value((format("Connection%02d") % numConnections).str().c_str());
        connectionNode.append_attribute("tags").set_value("part");

        pugi::xml_node partsNode = connectionNode.append_child("parts");
        partNode = partsNode.append_child("part");
        partNode.append_attribute("name").set_value(Name.c_str());
    }
    WritePartParent(partNode);
    WritePartPosition(partNode);

    // special case - ref parts don't have a size or center or lods
    // TODO blender tag?
    if (partNode.attribute("ref")) {
        return;
    }
    pugi::xml_node sizeNode = partNode.select_node("size").node();
    if (!sizeNode) {
        sizeNode = partNode.append_child("size");
    }
    WritePartSize(sizeNode);
    WritePartCenter(sizeNode);
    WritePartLods(partNode, geometryFolderPath, pIOHandler);
    if (CollisionMesh) {
        //TODO better solution
        std::string xmfFileName = PathUtil::GetOutputPath((format("%s-collision.xmf") % Name).str());
        std::string xmfFilePath = (geometryFolderPath / xmfFileName).string();
        CollisionMesh->WriteToFile(xmfFilePath, pIOHandler);
    }
}

void Part::WritePartParent(pugi::xml_node partNode) {
    pugi::xml_node connectionNode = partNode.parent().parent();
    pugi::xml_attribute parentAttr = connectionNode.attribute("parent");
    if (ParentName.empty()) {
        if (parentAttr) {
            connectionNode.remove_attribute(parentAttr);
        }
    } else {
        if (!parentAttr) {
            parentAttr = connectionNode.append_attribute("parent");
        }
        parentAttr.set_value(ParentName.c_str());
    }
}

void Part::WritePartPosition(pugi::xml_node partNode) {
    pugi::xml_node connectionNode = partNode.parent().parent();
    pugi::xml_node offsetNode = connectionNode.select_node("offset").node();
    if (!offsetNode) {
        offsetNode = connectionNode.append_child("offset");
    }
    pugi::xml_node positionNode = offsetNode.select_node("position").node();
    if (!positionNode) {
        positionNode = offsetNode.append_child("position");
        positionNode.append_attribute("x");
        positionNode.append_attribute("y");
        positionNode.append_attribute("z");
    }
    positionNode.attribute("x").set_value((format("%f") % Position.x).str().c_str());
    positionNode.attribute("y").set_value((format("%f") % Position.y).str().c_str());
    positionNode.attribute("z").set_value((format("%f") % Position.z).str().c_str());
}

void Part::WritePartSize(pugi::xml_node sizeNode) {
    pugi::xml_node maxNode = sizeNode.select_node("max").node();
    if (!maxNode) {
        maxNode = sizeNode.append_child("max");
        maxNode.append_attribute("x");
        maxNode.append_attribute("y");
        maxNode.append_attribute("z");
    }
    maxNode.attribute("x").set_value(Size.x);
    maxNode.attribute("y").set_value(Size.y);
    maxNode.attribute("z").set_value(Size.z);
}

void Part::WritePartCenter(pugi::xml_node sizeNode) {
    pugi::xml_node centerNode = sizeNode.select_node("center").node();
    if (!centerNode) {
        centerNode = sizeNode.append_child("center");
        centerNode.append_attribute("x");
        centerNode.append_attribute("y");
        centerNode.append_attribute("z");
    }
    centerNode.attribute("x").set_value(Center.x);
    centerNode.attribute("y").set_value(Center.y);
    centerNode.attribute("z").set_value(Center.z);
}

void Part::WritePartLods(pugi::xml_node partNode, const boost::filesystem::path &geometryFolderPath,
                                  Assimp::IOSystem *pIOHandler) {
    pugi::xml_node lodsNode = partNode.select_node("lods").node();
    if (!lodsNode)
        lodsNode = partNode.append_child("lods");

    // Remove LOD's that are no longer in the part
    pugi::xpath_node_set lodNodes = lodsNode.select_nodes("lod");
    for (auto lodNode : lodNodes) {
        int lodIndex = lodNode.node().attribute("index").as_int();
        if (!GetLod(lodIndex))
            lodsNode.remove_child(lodNode.node());
    }

    // Add/update remaining LOD's
    for (Lod &lod: Lods) {
        pugi::xml_node lodNode = lodsNode.select_node((format("lod[@index='%d']") % lod.Index).str().c_str()).node();
        if (!lodNode) {
            lodNode = lodsNode.append_child("lod");
            lodNode.append_attribute("index").set_value(lod.Index);
        }

        // Ensure material node
        pugi::xml_node materialsNode = lodNode.select_node("materials").node();
        if (!materialsNode)
            materialsNode = lodNode.append_child("materials");

        // Remove materials that are no longer in the lod
        pugi::xpath_node_set materialNodes = materialsNode.select_nodes("material");
        for (auto materialNode : materialNodes) {
            if (materialNode.node().attribute("id").as_int() > lod.Mesh->NumMaterials())
                materialsNode.remove_child(materialNode.node());
        }

        // Add/update remaining materials
        int materialId = 1;
        for (XmfMaterial &material: lod.Mesh->GetMaterials()) {
            pugi::xml_node materialNode = materialsNode.select_node(
                    (format("material[@id='%d']") % materialId).str().c_str()).node();
            if (!materialNode) {
                materialNode = materialsNode.append_child("material");
                materialNode.append_attribute("id").set_value(materialId);
            }
            pugi::xml_attribute refAttr = materialNode.attribute("ref");
            if (!refAttr) {
                refAttr = materialNode.append_attribute("ref");
            }
            refAttr.set_value(material.Name);
            materialId++;

        }

        // Write mesh file
        std::string xmfFileName = PathUtil::GetOutputPath((format("%s-lod%d.xmf") % Name % lod.Index).str());
        std::string xmfFilePath = (geometryFolderPath / xmfFileName).string();
        lod.Mesh->WriteToFile(xmfFilePath, pIOHandler);
    }
}