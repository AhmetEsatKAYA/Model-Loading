#ifndef MODEL_H
#define MODEL_H

//Bu kodla texture ve materyal yükleyebilirsiniz.

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <learnopengl/filesystem.h>
#include <learnopengl/camera.h>
#include <learnopengl/shader_m.h>

// Load a texture from file
unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// Vertex structure for storing model data
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

// Mesh class to handle mesh data and drawing
class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int textureID;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, unsigned int textureID, glm::vec3 diffuse, glm::vec3 specular, float shininess) {
        this->vertices = vertices;
        this->indices = indices;
        this->textureID = textureID;
        this->diffuse = diffuse;
        this->specular = specular;
        this->shininess = shininess;
        setupMesh();
    }

    void Draw(Shader &shaderProgram) {
        if (textureID != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glUniform1i(glGetUniformLocation(shaderProgram.ID, "material.texture_diffuse1"), 0);
        } else {
            glUniform3fv(glGetUniformLocation(shaderProgram.ID, "material.diffuse"), 1, glm::value_ptr(diffuse));
        }
        glUniform3fv(glGetUniformLocation(shaderProgram.ID, "material.specular"), 1, glm::value_ptr(specular));
        glUniform1f(glGetUniformLocation(shaderProgram.ID, "material.shininess"), shininess);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

private:
    unsigned int VAO, VBO, EBO;

    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // Vertex Positions
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);
        // Vertex Normals
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        glEnableVertexAttribArray(1);
        // Vertex Texture Coords
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }
};


// Model class to load and manage a 3D model
class Model {
public:

    std::string directory;
    std::vector<Mesh> meshes;

    Model(const std::string &path) {

        loadModel(path);
    }

    void Draw(Shader &shaderProgram) {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shaderProgram);
        clearModelCache();
    }

private:

    void loadModel(const std::string &path) {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
            return;
        }
        directory = path.substr(0, path.find_last_of('/'));  // Texture path is stored in directory
        processNode(scene->mRootNode, scene);
    }


    void processNode(aiNode *node, const aiScene *scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    
    void clearModelCache() {
        // Bağlı olan texture'u sıfırlayın
        glBindTexture(GL_TEXTURE_2D, 0);

        // Vertex Array ve Buffer nesnelerini sıfırlayın
        glBindVertexArray(0);
    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        unsigned int textureID = 0;
        glm::vec3 diffuse(0.5f, 0.5f, 0.5f);
        glm::vec3 specular(0.5f, 0.5f, 0.5f);
        float shininess = 32.0f;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            if (mesh->mTextureCoords[0])
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        if (mesh->mMaterialIndex >= 0) {
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
            aiColor3D color(0.f, 0.f, 0.f);

            material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
            diffuse = glm::vec3(color.r, color.g, color.b);

            material->Get(AI_MATKEY_COLOR_SPECULAR, color);
            specular = glm::vec3(color.r, color.g, color.b);

            material->Get(AI_MATKEY_SHININESS, shininess);

            std::vector<std::string> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE);
            if (!diffuseMaps.empty()) {
                textureID = loadTexture(diffuseMaps[0].c_str());
            }
        }

        return Mesh(vertices, indices, textureID, diffuse, specular, shininess);
    }

    std::vector<std::string> loadMaterialTextures(aiMaterial *mat, aiTextureType type) {
        std::vector<std::string> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            std::string texturePath = directory + "/" + str.C_Str(); // Combine the directory and texture file name
            textures.push_back(texturePath);
        }
        return textures;
    }

};

#endif
