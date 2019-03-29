#include <X4ConverterTools/xmf/XmfImporter.h>

using namespace boost;
using namespace boost::algorithm;
using namespace boost::filesystem;
using namespace Assimp;


namespace xmf {
    XmfImporter::XmfImporter(const std::string &gameBaseFolderPath) :
            _materialLibrary(gameBaseFolderPath) {
        _gameBaseFolderPath = gameBaseFolderPath;
    }

    const aiImporterDesc *XmfImporter::GetInfo() const {
        static aiImporterDesc info;
        if (!info.mAuthor) {
            info.mName = "EgoSoft XuMeshFile importer";
            info.mAuthor = "arc_";
            info.mFileExtensions = "xml";
            info.mMinMajor = 3;
            info.mMinMinor = 0;
            info.mMaxMajor = 3;
            info.mMaxMinor = 0;
        }
        return &info;
    }

    bool XmfImporter::CanRead(const std::string &filePath, IOSystem *pIOHandler,
                              bool checkSig) const {
        return iends_with(filePath, ".xml");
    }

    void XmfImporter::InternReadFile(const std::string &filePath, aiScene *pScene,
                                     IOSystem *pIOHandler) {
        try {
            // Read the .xml and .xmf files
            std::shared_ptr<Component> pComponent = Component::ReadFromFile(
                    filePath, _gameBaseFolderPath, pIOHandler);

            // Convert to the Assimp data model
            ConversionContext context;
            pScene->mRootNode = ConvertComponentToAiNode(*pComponent, context);

            // Add the meshes to the scene
            pScene->mMeshes = new aiMesh *[context.Meshes.size()];
            for (aiMesh *pMesh : context.Meshes) {
                AssimpUtil::MergeVertices(pMesh);
                pScene->mMeshes[pScene->mNumMeshes++] = pMesh;
            }

            // TODO
//        // ANI file stuff
            // TODO more robust
            std::string shortName = path(filePath).filename().replace_extension("").string() + "_data.ani";
            to_upper(shortName);
            std::string aniPath = (path(filePath).parent_path() / shortName).string();
            IOStream *pAniStream = pIOHandler->Open(aniPath, "r");
            if (pAniStream == nullptr) {
                std::cerr << "No ANI file fond at path: " << aniPath << ". This likely indicates an error."
                          << std::endl;
            } else {
                ani::AnimFile aniFile(pAniStream);
            }
//        pScene->mNumAnimations = aniFile.getHeader().getNumAnims();
//
            AddMaterials(filePath, pScene, context);

        } catch (std::exception &e) {
            std::cerr << e.what() << std::endl;
            throw DeadlyImportError(e.what());
        }
    }

    void XmfImporter::AddMaterials(const std::string &filePath, aiScene *pScene, const ConversionContext &context) {
        // Add the materials to the scene
        if (!context.Materials.empty()) {
            std::string modelFolderPath = path(filePath).parent_path().string();
            pScene->mNumMaterials = numeric_cast<uint>(context.Materials.size());
            pScene->mMaterials = new aiMaterial *[pScene->mNumMaterials];
            for (auto &it : context.Materials) {
                Material *pMaterial = _materialLibrary.GetMaterial(it.first);
                aiMaterial *pAiMaterial;
                if (pMaterial) {
                    pAiMaterial = pMaterial->ConvertToAiMaterial(modelFolderPath, _gameBaseFolderPath);
                } else {
                    std::cerr << "Warning, weird case" << std::endl;
                    auto *tempString = new aiString(it.first);
                    pAiMaterial = new aiMaterial();
                    pAiMaterial->AddProperty(tempString, AI_MATKEY_NAME);
                    delete tempString;
                }
                pScene->mMaterials[it.second] = pAiMaterial;
            }
        }
    }

    aiNode *XmfImporter::ConvertComponentToAiNode(Component &component,
                                                  ConversionContext &context) {
        std::map<std::string, aiNode *> partNodes;

        // Create nodes and meshes
        for (auto &Part : component.Parts) {
            partNodes[Part.first] = ConvertComponentPartToAiNode(Part.second,
                                                                 context);
        }

        // Link parent nodes
        std::vector<aiNode *> rootNodes;
        std::map<aiNode *, std::vector<aiNode *> > nodeChildren;
        for (auto &Part : component.Parts) {
            aiNode *pPartNode = partNodes[Part.first];
            if (Part.second.ParentName.empty()) {
                rootNodes.push_back(pPartNode);
                continue;
            }

            auto parentIt = partNodes.find(Part.second.ParentName);
            if (parentIt == partNodes.end()) {
                throw std::runtime_error(
                        str(
                                format("Node %1% has invalid parent %2%") % (Part.first).c_str()
                                % (Part.second.ParentName).c_str()));
            }
            nodeChildren[parentIt->second].push_back(pPartNode);
        }

        for (auto &it : nodeChildren) {
            aiNode *pParentNode = it.first;
            auto **ppNewChildren = new aiNode *[pParentNode->mNumChildren
                                                + it.second.size()];
            if (pParentNode->mChildren) {
                memcpy(ppNewChildren, pParentNode->mChildren,
                       sizeof(aiNode *) * pParentNode->mNumChildren);
                delete[] pParentNode->mChildren;
            }
            pParentNode->mChildren = ppNewChildren;

            for (aiNode *pChildNode : it.second) {
                pParentNode->mChildren[pParentNode->mNumChildren++] = pChildNode;
                pChildNode->mParent = pParentNode;
            }
        }

// Create component node that contains all the part nodes
        if (rootNodes.empty())
            throw std::runtime_error("No root parts found");

        auto *pComponentNode = new aiNode(component.Name);
        pComponentNode->mChildren = new aiNode *[rootNodes.size()];

        aiNode *pRealRootNode = new aiNode("Scene");// To make blender happy name root node Scene?
        pRealRootNode->mChildren = new aiNode *[1];
        pRealRootNode->mChildren[0] = pComponentNode;
        pRealRootNode->mNumChildren = 1;
        pComponentNode->mParent = pRealRootNode;
        for (int i = 0; i < rootNodes.size(); i++) {
            aiNode *pRootNode = rootNodes[i];
            pComponentNode->mChildren[i] = pRootNode;
            pRootNode->mParent = pComponentNode;
            pComponentNode->mNumChildren++;
        }
        return pRealRootNode;
//    return pComponentNode;
    }

    aiNode *XmfImporter::ConvertComponentPartToAiNode(ComponentPart &part,
                                                      ConversionContext &context) {
        auto *pPartNode = new aiNode();
        try {
            pPartNode->mName = part.Name;
            pPartNode->mTransformation.a4 = -part.Position.x;
            pPartNode->mTransformation.b4 = part.Position.y;
            pPartNode->mTransformation.c4 = part.Position.z;

            pPartNode->mChildren = new aiNode *[part.Lods.size()
                                                + (part.CollisionMesh ? 1 : 0)];

            for (ComponentPartLod &lod : part.Lods) {
                pPartNode->mChildren[pPartNode->mNumChildren++] = lod.Mesh->ConvertXuMeshToAiNode(
                        (format("%sXlod%d") % part.Name % lod.LodIndex).str(), context);
            }
            if (part.CollisionMesh)
                pPartNode->mChildren[pPartNode->mNumChildren++] = part.CollisionMesh->ConvertXuMeshToAiNode(
                        part.Name + "Xcollision", context);
        } catch (...) {
            // TODO real exception handling
            delete pPartNode;
            throw;
        }
        return pPartNode;
    }

}