#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

#include "shader.h"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};

struct Texture {
    GLuint      id   { 0 };
    std::string type;
    std::string path;
};

class Mesh
{
public:
    std::vector<Vertex>  vertices;
    std::vector<GLuint>  indices;
    std::vector<Texture> textures;

    Mesh(std::vector<Vertex>  v,
         std::vector<GLuint>  i,
         std::vector<Texture> t)
        : vertices(std::move(v))
        , indices (std::move(i))
        , textures(std::move(t))
    {
        setupGPU();
    }

    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&)                 = default;
    Mesh& operator=(Mesh&&)      = default;

    ~Mesh()
    {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);
    }

    void draw(Shader& shader) const
    {
        unsigned diffN = 0, specN = 0, emisN = 0;

        for (unsigned i = 0; i < textures.size(); ++i) {
            glActiveTexture(GL_TEXTURE0 + i);

            std::string name;
            if (textures[i].type == "texture_diffuse")
                name = "material.diffuse"  + std::to_string(diffN++);
            else if (textures[i].type == "texture_specular")
                name = "material.specular" + std::to_string(specN++);
            else if (textures[i].type == "texture_emissive")
                name = "material.emissive" + std::to_string(emisN++);

            shader.setInt(name, i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        shader.setInt("material.hasDiffuse",  diffN > 0 ? 1 : 0);
        shader.setInt("material.hasSpecular", specN > 0 ? 1 : 0);
        shader.setInt("material.hasEmissive", emisN > 0 ? 1 : 0);

        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

        glActiveTexture(GL_TEXTURE0);
    }

private:
    GLuint m_vao { 0 };
    GLuint m_vbo { 0 };
    GLuint m_ebo { 0 };

    void setupGPU()
    {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ebo);

        glBindVertexArray(m_vao);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     vertices.size() * sizeof(Vertex),
                     vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     indices.size() * sizeof(GLuint),
                     indices.data(), GL_STATIC_DRAW);

        // position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, position));
        // normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, normal));
        // texcoords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (void*)offsetof(Vertex, texCoords));

        glBindVertexArray(0);
    }
};
