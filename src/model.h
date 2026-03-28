#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "mesh.h"
#include "shader.h"

namespace fs = std::filesystem;

class Model
{
public:
    explicit Model(const std::string& path)
    {
        m_directory = fs::path(path).parent_path().string();
        load(path);
    }

    void draw(Shader& shader) const
    {
        for (const auto& mesh : m_meshes)
            mesh.draw(shader);
    }

    bool isLoaded() const { return m_loaded; }

private:
    std::vector<Mesh>                        m_meshes;
    std::string                              m_directory;
    std::unordered_map<std::string, Texture> m_texCache;
    bool                                     m_loaded { false };

    void load(const std::string& path)
    {
        Assimp::Importer importer;

        unsigned flags = aiProcess_Triangulate
                       | aiProcess_FlipUVs
                       | aiProcess_CalcTangentSpace
                       | aiProcess_GenNormals
                       | aiProcess_JoinIdenticalVertices;

        const aiScene* scene = importer.ReadFile(path, flags);

        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
            std::cerr << "[Model] Assimp error loading \"" << path
                      << "\": " << importer.GetErrorString() << "\n";
            return;
        }

        processNode(scene->mRootNode, scene);
        m_loaded = true;
        std::cout << "[Model] Loaded: " << path
                  << "  (" << m_meshes.size() << " meshes)\n";
    }

    void processNode(aiNode* node, const aiScene* scene)
    {
        for (unsigned i = 0; i < node->mNumMeshes; ++i)
            m_meshes.push_back(processMesh(scene->mMeshes[node->mMeshes[i]], scene));

        for (unsigned i = 0; i < node->mNumChildren; ++i)
            processNode(node->mChildren[i], scene);
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        std::vector<Vertex>  vertices;
        std::vector<GLuint>  indices;
        std::vector<Texture> textures;

        vertices.reserve(mesh->mNumVertices);
        for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
            Vertex v {};
            v.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

            if (mesh->HasNormals())
                v.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };

            if (mesh->mTextureCoords[0])
                v.texCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };

            vertices.push_back(v);
        }

        for (unsigned i = 0; i < mesh->mNumFaces; ++i) {
            const aiFace& face = mesh->mFaces[i];
            for (unsigned j = 0; j < face.mNumIndices; ++j)
                indices.push_back(face.mIndices[j]);
        }

        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

            auto diffuse  = loadMaterialTextures(mat, aiTextureType_DIFFUSE,  "texture_diffuse");
            auto specular = loadMaterialTextures(mat, aiTextureType_SPECULAR, "texture_specular");
            auto emissive = loadMaterialTextures(mat, aiTextureType_EMISSIVE, "texture_emissive");

            textures.insert(textures.end(), diffuse.begin(),  diffuse.end());
            textures.insert(textures.end(), specular.begin(), specular.end());
            textures.insert(textures.end(), emissive.begin(), emissive.end());
        }

        return Mesh(std::move(vertices), std::move(indices), std::move(textures));
    }

    // Try to find a file, attempting alternate extensions if needed
    static std::string tryAlternateExtensions(const std::string& path)
    {
        if (fs::exists(path)) return path;

        // Try swapping between common image extensions
        static const std::vector<std::string> exts = { ".png", ".jpg", ".jpeg", ".tga", ".bmp" };
        fs::path p(path);
        std::string stem = p.stem().string();
        std::string dir  = p.parent_path().string();

        for (const auto& ext : exts) {
            std::string alt = dir + "/" + stem + ext;
            if (fs::exists(alt)) return alt;
        }
        return path; // return original if nothing found
    }

    std::vector<Texture> loadMaterialTextures(aiMaterial* mat,
                                              aiTextureType type,
                                              const std::string& typeName)
    {
        std::vector<Texture> out;

        for (unsigned i = 0; i < mat->GetTextureCount(type); ++i) {
            aiString aiPath;
            mat->GetTexture(type, i, &aiPath);

            // Strip inline comments (e.g. "file.png  # comment")
            std::string rawPath = aiPath.C_Str();
            auto hashPos = rawPath.find('#');
            if (hashPos != std::string::npos)
                rawPath = rawPath.substr(0, hashPos);
            // Trim trailing whitespace
            while (!rawPath.empty() && (rawPath.back() == ' ' || rawPath.back() == '\t' || rawPath.back() == '\r'))
                rawPath.pop_back();

            std::string filename = fs::path(rawPath).filename().string();
            std::string fullPath = tryAlternateExtensions(m_directory + "/" + rawPath);

            if (!fs::exists(fullPath))
                fullPath = tryAlternateExtensions(m_directory + "/" + filename);

            if (!fs::exists(fullPath)) {
                // Recursive search in model directory for the filename (try alt extensions)
                for (auto& entry : fs::recursive_directory_iterator(m_directory)) {
                    std::string entryName = entry.path().filename().string();
                    std::string entryStem = entry.path().stem().string();
                    std::string fileStem  = fs::path(filename).stem().string();
                    if (entryName == filename || entryStem == fileStem) {
                        fullPath = entry.path().string();
                        break;
                    }
                }
            }

            auto it = m_texCache.find(fullPath);
            if (it != m_texCache.end()) {
                out.push_back(it->second);
            } else {
                GLuint texId = loadTextureFromFile(fullPath);
                Texture tex { texId, typeName, fullPath };
                m_texCache[fullPath] = tex;
                out.push_back(tex);
            }
        }

        return out;
    }

    static GLuint loadTextureFromFile(const std::string& path)
    {
        if (!fs::exists(path)) {
            std::cerr << "[Model] Texture not found: " << path
                      << "  (using gray fallback)\n";
            return createFallbackTexture();
        }

        int w, h, channels;
        stbi_set_flip_vertically_on_load(false);
        unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 0);

        if (!data) {
            std::cerr << "[Model] stb_image failed: " << path << "\n";
            return createFallbackTexture();
        }

        GLenum fmt = GL_RGB;
        if      (channels == 1) fmt = GL_RED;
        else if (channels == 4) fmt = GL_RGBA;

        GLuint id;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, (GLint)fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        return id;
    }

    static GLuint createFallbackTexture()
    {
        GLuint id;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        unsigned char gray[3] = { 200, 200, 200 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, gray);
        glGenerateMipmap(GL_TEXTURE_2D);
        return id;
    }
};
