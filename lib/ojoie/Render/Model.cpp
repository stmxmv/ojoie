////
//// Created by Aleudillonam on 8/7/2022.
////
//#include "Render/Model.hpp"
//#include <glad/glad.h>
//
//#include <assimp/Importer.hpp>
//#include <assimp/postprocess.h>
//#include <assimp/scene.h>
//
//#include "Render//TextureLoader.hpp"
//
//#include <format>
//
//namespace AN {
//
//struct ModelInitContext {
//    std::string directory;
//    std::vector<SubMesh> &meshes;
//    std::vector<ModelTexture> &loadedTextures;
//    std::vector<Vertex> vertices;
//    std::vector<unsigned int> indices;
//};
//
//
//std::vector<TextureInfo> loadMaterialTextures(ModelInitContext &context, aiMaterial *mat, aiTextureType type, TextureType meshTextureType, const aiScene *scene) {
//    std::vector<TextureInfo> textures;
//    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
//        aiString str;
//        mat->GetTexture(type, i, &str);
//        // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
//        bool skip = false;
//        for (auto &loadedTexture : context.loadedTextures) {
//            if (std::strcmp(loadedTexture.path.data(), str.C_Str()) == 0) {
//                textures.push_back({loadedTexture.id, loadedTexture.type});
//                skip = true;// a texture with the same filepath has already been loaded, continue to next one. (optimization)
//                break;
//            }
//        }
//        if (!skip) {// if texture hasn't been loaded already, load it
//            ModelTexture texture{};
//            const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(str.C_Str());
//
//            if (embeddedTexture) {
//                uint64_t textureId;
//                if (embeddedTexture->mHeight != 0) {
//                    textureId = STBTextureLoader::loadTextureFromMemory((const unsigned char *)embeddedTexture->pcData, embeddedTexture->mHeight * embeddedTexture->mWidth);
//                } else {
//                    textureId = STBTextureLoader::loadTextureFromMemory((const unsigned char *)embeddedTexture->pcData, embeddedTexture->mWidth);
//                }
//
//                texture.id = textureId;
//
//            } else {
//                std::string path = context.directory + "/" + str.C_Str();
//                texture.id       = STBTextureLoader::loadTexture(path.c_str());
//            }
//
//            if (texture.id == 0) {
//
//                ANLog("Load texture fail!");
//
//            }
//
//            texture.type     = meshTextureType;
//            texture.path     = str.C_Str();
//            textures.push_back({texture.id, texture.type});
//            context.loadedTextures.push_back(texture);// store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
//        }
//    }
//    return textures;
//}
//
//SubMesh processMesh(ModelInitContext &context, aiMesh *mesh, const aiScene *scene) {
//    // data to fill
//    std::vector<Vertex> vertices;
//    std::vector<unsigned int> indices;
//    std::vector<TextureInfo> textures;
//
//    // walk through each of the mesh's vertices
//    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
//        Vertex vertex;
//        Math::vec3 vector;// we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to Math's vec3 class so we transfer the data to this placeholder Math::vec3 first.
//        // positions
//        vector.x        = mesh->mVertices[i].x;
//        vector.y        = mesh->mVertices[i].y;
//        vector.z        = mesh->mVertices[i].z;
//        vertex.position = vector;
//        //        // normals
//        //        if (mesh->HasNormals())
//        //        {
//        //            vector.x = mesh->mNormals[i].x;
//        //            vector.y = mesh->mNormals[i].y;
//        //            vector.z = mesh->mNormals[i].z;
//        //            vertex.normal = vector;
//        //        }
//        // texture coordinates
//        if (mesh->mTextureCoords[0])// does the mesh contain texture coordinates?
//        {
//            Math::vec2 vec;
//            // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
//            // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
//            vec.x           = mesh->mTextureCoords[0][i].x;
//            vec.y           = mesh->mTextureCoords[0][i].y;
//            vertex.texCoord = vec;
//            //            // tangent
//            //            vector.x = mesh->mTangents[i].x;
//            //            vector.y = mesh->mTangents[i].y;
//            //            vector.z = mesh->mTangents[i].z;
//            //            vertex.Tangent = vector;
//            //            // bitangent
//            //            vector.x = mesh->mBitangents[i].x;
//            //            vector.y = mesh->mBitangents[i].y;
//            //            vector.z = mesh->mBitangents[i].z;
//            //            vertex.Bitangent = vector;
//        } else {
//            vertex.texCoord = Math::vec2(0.0f, 0.0f);
//        }
//
//        vertices.push_back(vertex);
//    }
//    // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
//    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
//        aiFace face = mesh->mFaces[i];
//        // retrieve all indices of the face and store them in the indices vector
//        for (unsigned int j = 0; j < face.mNumIndices; j++) {
//            indices.push_back(face.mIndices[j]);
//        }
//    }
//    // process materials
//    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
//    // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
//    // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
//    // Same applies to other texture as the following list summarizes:
//    // diffuse: texture_diffuseN
//    // specular: texture_specularN
//    // normal: texture_normalN
//
//    // 1. diffuse maps
//    std::vector<TextureInfo> diffuseMaps = loadMaterialTextures(context, material, aiTextureType_DIFFUSE, TextureType::diffuse, scene);
//    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
//    // 2. specular maps
//    std::vector<TextureInfo> specularMaps = loadMaterialTextures(context, material, aiTextureType_SPECULAR, TextureType::specular, scene);
//    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
//    // 3. normal maps
//    std::vector<TextureInfo> normalMaps = loadMaterialTextures(context, material, aiTextureType_HEIGHT, TextureType::normal, scene);
//    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
//    // 4. height maps
//    std::vector<TextureInfo> heightMaps = loadMaterialTextures(context, material, aiTextureType_AMBIENT, TextureType::height, scene);
//    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
//
//    // return a mesh object created from the extracted mesh data
//
//
//
//
//    SubMesh subMesh;
//    subMesh.indexCount = indices.size();
//    subMesh.indexOffset = context.indices.size();
//    subMesh.textures = textures;
//
//    for (auto &index : indices) {
//        index += context.vertices.size();
//    }
//
//    context.vertices.insert(context.vertices.end(), vertices.begin(), vertices.end());
//
//    context.indices.insert(context.indices.end(), indices.begin(), indices.end());
//
//    return subMesh;
//}
//
//
//void processNode(ModelInitContext &context, aiNode *node, const aiScene *scene) {
//    // process each mesh located at the current node
//    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
//        // the node object only contains indices to index the actual objects in the scene.
//        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
//        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
//        context.meshes.push_back(processMesh(context, mesh, scene));
//    }
//    // after we've processed all the meshes (if any) we then recursively process each of the children nodes
//    for (unsigned int i = 0; i < node->mNumChildren; i++) {
//        processNode(context, node->mChildren[i], scene);
//    }
//}
//
//struct Model::Impl {
//    GLuint vao;
//    GLuint vbo;
//    GLuint ebo;
//};
//
//Model::Model() : impl(new Impl{}) {}
//
//Model::~Model() {
//    for (auto &texture : loadedTextures) {
//        unsigned int id = (unsigned int)texture.id;
//        glDeleteTextures(1, &id);
//    }
//
//    glDeleteBuffers(1, &impl->vbo);
//    glDeleteBuffers(1, &impl->ebo);
//    glDeleteVertexArrays(1, &impl->vao);
//
//    delete impl;
//}
//
//bool Model::init(const char *modelPath) {
//
//    Assimp::Importer importer;
//    const aiScene *scene = importer.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
//    // check for errors
//    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
//        ANLog("ERROR::ASSIMP:: %s", importer.GetErrorString());
//        return false;
//    }
//    // retrieve the directory path of the filepath
//    std::string_view modelPathView(modelPath);
//    ModelInitContext context{
//            .directory      = std::string(modelPathView.substr(0, modelPathView.find_last_of('/'))),
//            .meshes         = meshes,
//            .loadedTextures = loadedTextures
//    };
//
//
//    // process ASSIMP's root node recursively
//    processNode(context, scene->mRootNode, scene);
//
//    glGenVertexArrays(1, &impl->vao);
//    glGenBuffers(1, &impl->vbo);
//    glGenBuffers(1, &impl->ebo);
//
//    glBindVertexArray(impl->vao);
//
//    glBindBuffer(GL_ARRAY_BUFFER, impl->vbo);
//    glBufferData(GL_ARRAY_BUFFER, context.vertices.size() * sizeof(Vertex), context.vertices.data(), GL_STATIC_DRAW);
//
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, impl->ebo);
//    glBufferData(GL_ELEMENT_ARRAY_BUFFER, context.indices.size() * sizeof(uint32_t), context.indices.data(), GL_STATIC_DRAW);
//
//    // position attribute
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
//    glEnableVertexAttribArray(0);
//    // texture coord attribute
//    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, texCoord)));
//    glEnableVertexAttribArray(1);
//
//
//    return true;
//}
//
//void Model::render(const RC::RenderPipeline &pipeline) {
//
//    glBindVertexArray(impl->vao);
//
//    for (auto &subMesh: meshes) {
//        unsigned int diffuseNr = 1;
//        unsigned int specularNr = 1;
//        for (unsigned int i = 0; i < (unsigned int)subMesh.textures.size(); i++) {
//            glActiveTexture(GL_TEXTURE0 + i); // activate proper texture unit before binding
//            // retrieve texture number (the N in diffuse_textureN)
//            unsigned int number;
//            const char * name;
//            switch (subMesh.textures[i].type) {
//                case TextureType::diffuse:
//                    name = "texture_diffuse";
//                    number = diffuseNr++;
//                    break;
//                case TextureType::specular:
//                    name = "texture_specular";
//                    number = specularNr++;
//                    break;
//                default:
//                    continue;
//            }
//
////            pipeline.setInt(std::format("material.{}{}", name, number).c_str(), i);
//
//            glBindTexture(GL_TEXTURE_2D, subMesh.textures[i].id);
//        }
////        glDrawRangeElements(GL_TRIANGLES, subMesh.indexOffset, subMesh.indexOffset + subMesh.indexCount, subMesh.indexCount, GL_UNSIGNED_INT, 0);
//        glDrawElements(GL_TRIANGLES, subMesh.indexCount, GL_UNSIGNED_INT, (void *)(subMesh.indexOffset * sizeof(uint32_t)));
//    }
//
//
//}
//
//}// namespace AN