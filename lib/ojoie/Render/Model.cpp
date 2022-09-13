//
// Created by Aleudillonam on 8/7/2022.
//
#include "Render/Model.hpp"
#include <glad/glad.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <ojoie/Render/Renderer.hpp>

#include "Render/Sampler.hpp"
#include "Render//TextureLoader.hpp"

#include <format>

namespace AN {

struct ModelInitContext {
    std::string directory;
    std::vector<SubMesh> &meshes;
    std::vector<ModelTexture> &loadedTextures;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

static RC::Sampler sampler;

std::vector<ModelSubMeshTexture> loadMaterialTextures(ModelInitContext &context, aiMaterial *mat, aiTextureType type, TextureType meshTextureType, const aiScene *scene) {
    std::vector<ModelSubMeshTexture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
        bool skip = false;
        for (uint64_t j = 0; j < context.loadedTextures.size(); ++j) {
            if (std::strcmp(context.loadedTextures[j].path.c_str(), str.C_Str()) == 0) {
                textures.push_back({ j, context.loadedTextures[j].type });
                skip = true;// a texture with the same filepath has already been loaded, continue to next one. (optimization)
                break;
            }
        }
        if (!skip) {// if texture hasn't been loaded already, load it
            ModelTexture modelTexture{};
            const aiTexture* embeddedTexture = scene->GetEmbeddedTexture(str.C_Str());

            if (embeddedTexture) {
                if (embeddedTexture->mHeight != 0) {
                    modelTexture.texture = TextureLoader::loadTextureFromMemory((const unsigned char *)embeddedTexture->pcData, embeddedTexture->mHeight * embeddedTexture->mWidth);
                } else {
                    modelTexture.texture = TextureLoader::loadTextureFromMemory((const unsigned char *)embeddedTexture->pcData, embeddedTexture->mWidth);
                }

                ANLog("Loaded embedded texture %s", str.C_Str());

            } else {
                std::string path = context.directory + "/" + str.C_Str();
                modelTexture.texture = TextureLoader::loadTexture(path.c_str());
                ANLog("Loaded external texture %s at %s", str.C_Str(), path.c_str());
            }

            modelTexture.type     = meshTextureType;
            modelTexture.path     = str.C_Str();
            textures.push_back({ context.loadedTextures.size(), modelTexture.type});

            context.loadedTextures.push_back(std::move(modelTexture));// store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
        }
    }
    return textures;
}

SubMesh processMesh(ModelInitContext &context, aiMesh *mesh, const aiScene *scene) {
    // data to fill
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<ModelSubMeshTexture> textures;

    // walk through each of the mesh's vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        Math::vec3 vector;// we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to Math's vec3 class so we transfer the data to this placeholder Math::vec3 first.
        // positions
        vector.x        = mesh->mVertices[i].x;
        vector.y        = mesh->mVertices[i].y;
        vector.z        = mesh->mVertices[i].z;
        vertex.position = vector;
                // normals
        if (mesh->HasNormals()) {
            vector.x      = mesh->mNormals[i].x;
            vector.y      = mesh->mNormals[i].y;
            vector.z      = mesh->mNormals[i].z;
            vertex.normal = vector;
        }
        // texture coordinates
        if (mesh->mTextureCoords[0])// does the mesh contain texture coordinates?
        {
            Math::vec2 vec;
            // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
            // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
            vec.x           = mesh->mTextureCoords[0][i].x;
            vec.y           = mesh->mTextureCoords[0][i].y;
            vertex.texCoord = vec;
            //            // tangent
            //            vector.x = mesh->mTangents[i].x;
            //            vector.y = mesh->mTangents[i].y;
            //            vector.z = mesh->mTangents[i].z;
            //            vertex.Tangent = vector;
            //            // bitangent
            //            vector.x = mesh->mBitangents[i].x;
            //            vector.y = mesh->mBitangents[i].y;
            //            vector.z = mesh->mBitangents[i].z;
            //            vertex.Bitangent = vector;
        } else {
            vertex.texCoord = Math::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }
    // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }
    // process materials
    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
    // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
    // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
    // Same applies to other texture as the following list summarizes:
    // diffuse: texture_diffuseN
    // specular: texture_specularN
    // normal: texture_normalN

    // 1. diffuse maps
    std::vector<ModelSubMeshTexture> diffuseMaps = loadMaterialTextures(context, material, aiTextureType_DIFFUSE, TextureType::diffuse, scene);
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    // 2. specular maps
    std::vector<ModelSubMeshTexture> specularMaps = loadMaterialTextures(context, material, aiTextureType_SPECULAR, TextureType::specular, scene);
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    // 3. normal maps
    std::vector<ModelSubMeshTexture> normalMaps = loadMaterialTextures(context, material, aiTextureType_HEIGHT, TextureType::normal, scene);
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
    // 4. height maps
    std::vector<ModelSubMeshTexture> heightMaps = loadMaterialTextures(context, material, aiTextureType_AMBIENT, TextureType::height, scene);
    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

    // return a mesh object created from the extracted mesh data




    SubMesh subMesh;
    subMesh.indexCount = (uint32_t)indices.size();
    subMesh.indexOffset = (uint32_t)context.indices.size();
    subMesh.vertexOffset = (uint32_t)context.vertices.size();
    subMesh.textures = textures;

    context.vertices.insert(context.vertices.end(), vertices.begin(), vertices.end());
    context.indices.insert(context.indices.end(), indices.begin(), indices.end());

    return subMesh;
}


void processNode(ModelInitContext &context, aiNode *node, const aiScene *scene) {
    // process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        // the node object only contains indices to index the actual objects in the scene.
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        context.meshes.push_back(processMesh(context, mesh, scene));
    }
    // after we've processed all the meshes (if any) we then recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(context, node->mChildren[i], scene);
    }
}



void Model::deinit() {
    for (auto &texture : loadedTextures) {
        texture.texture.deinit();
    }
    vertexBuffer.deinit();
    indexBuffer.deinit();
    lightUniformBuffer.deinit();
}

struct LightUniform {
    alignas(16) Math::vec3 lightPos;
    alignas(16) Math::vec3 lightColor;
};

bool Model::init(const char *modelPath) {

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(modelPath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        ANLog("ERROR::ASSIMP:: %s", importer.GetErrorString());
        return false;
    }
    // retrieve the directory path of the filepath
    std::string_view modelPathView(modelPath);
    ModelInitContext context{
            .directory      = std::string(modelPathView.substr(0, modelPathView.find_last_of('/'))),
            .meshes         = meshes,
            .loadedTextures = loadedTextures
    };


    // process ASSIMP's root node recursively
    processNode(context, scene->mRootNode, scene);

    const AN::RenderContext &renderContext = GetRenderer().getRenderContext();

    uint64_t verticesBytes = context.vertices.size() * sizeof(Vertex);
    uint64_t indexBytes = context.indices.size() * sizeof(uint32_t);

    RC::BufferBlock stageBufferBlock = renderContext.stageBufferPool.bufferBlock(verticesBytes + indexBytes);
    RC::BufferAllocation stageBufferAllocation = stageBufferBlock.allocate(verticesBytes + indexBytes);

    void *stageBufferData = stageBufferAllocation.map();
    memcpy(stageBufferData, context.vertices.data(), verticesBytes);
    memcpy((char *)stageBufferData + verticesBytes, context.indices.data(), indexBytes);

    stageBufferAllocation.getBuffer().flush();

    if (!vertexBuffer.init(verticesBytes)) {
        return false;
    }

    if (!indexBuffer.init(indexBytes)) {
        return false;
    }

    RC::BlitCommandEncoder &blitCommandEncoder = renderContext.blitCommandEncoder;

    blitCommandEncoder.copyBufferToBuffer(stageBufferAllocation.getBuffer(),
                                          stageBufferAllocation.getOffset() + 0, vertexBuffer.getBuffer(), 0, verticesBytes);

    blitCommandEncoder.copyBufferToBuffer(stageBufferAllocation.getBuffer(),
                                          stageBufferAllocation.getOffset() + verticesBytes, indexBuffer.getBuffer(), 0, indexBytes);

    RC::BufferMemoryBarrier bufferMemoryBarrier;
    bufferMemoryBarrier.srcStageFlag = RC::PipelineStageFlag::Transfer;
    bufferMemoryBarrier.srcAccessMask = RC::PipelineAccessFlag::TransferWrite;
    bufferMemoryBarrier.dstStageFlag = RC::PipelineStageFlag::VertexShader;
    bufferMemoryBarrier.dstAccessMask = RC::PipelineAccessFlag::ShaderRead;

    bufferMemoryBarrier.offset = 0;
    bufferMemoryBarrier.size = verticesBytes;

    blitCommandEncoder.bufferMemoryBarrier(vertexBuffer.getBuffer(), bufferMemoryBarrier);

    bufferMemoryBarrier.size = indexBytes;
    blitCommandEncoder.bufferMemoryBarrier(indexBuffer.getBuffer(), bufferMemoryBarrier);


    if (!lightUniformBuffer.init(sizeof(LightUniform))) {
        return false;
    }

    static bool samplerInited = false;
    if (!samplerInited) {
        samplerInited = true;
        RC::SamplerDescriptor samplerDescriptor = RC::SamplerDescriptor::Default();

        if (!sampler.init(samplerDescriptor)) {
            return false;
        }

        GetRenderQueue().registerCleanupTask([] {
            sampler.deinit();
        });
    }


    return true;
}

void Model::render() {

    RC::RenderCommandEncoder &renderCommandEncoder = GetRenderer().getRenderContext().renderCommandEncoder;

    renderCommandEncoder.bindSampler(2, sampler);

    LightUniform *uniform = (LightUniform *)(lightUniformBuffer.content());
    uniform->lightPos = { 1.2f, 10.0f, 2.0f };
    uniform->lightColor = { 0.980f, 0.976f, 0.902f };

    renderCommandEncoder.bindUniformBuffer(1, lightUniformBuffer.getOffset(), lightUniformBuffer.getSize(), lightUniformBuffer.getBuffer());

    renderCommandEncoder.bindVertexBuffer(0, vertexBuffer.getBufferOffset(0), vertexBuffer.getBuffer());

    renderCommandEncoder.bindIndexBuffer(RC::IndexType::UInt32, indexBuffer.getBufferOffset(0), indexBuffer.getBuffer());

    for (auto &subMesh: meshes) {
        for (unsigned int i = 0; i < (unsigned int)subMesh.textures.size(); i++) {
            switch (subMesh.textures[i].type) {
                case TextureType::diffuse:
                    renderCommandEncoder.bindTexture(3, loadedTextures[subMesh.textures[i].index].texture);
                    goto didBind;
                    break;
                case TextureType::specular:
                    break;
                default:
                    continue;
            }
        }

    didBind:


        renderCommandEncoder.drawIndexed(subMesh.indexCount, subMesh.indexOffset, subMesh.vertexOffset);
    }


}

}// namespace AN