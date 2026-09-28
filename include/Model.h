#ifndef MODEL_H
#define MODEL_H

#include "EricStructure.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Shader.h"

#include <tiny_obj_loader.h>
#include <iostream>
#include <vector>
#include <string>

// Assume loadTexture is declared somewhere or we provide it
extern unsigned int loadTexture(char const * path);

struct SubMesh {
    EricStructure geometry;
    VAO* vao;
    VBO* vbo;
    EBO* ebo;
    
    unsigned int diffuseMap;
    unsigned int specularMap;
    unsigned int normalMap;

    glm::vec3 diffuseColor;
    glm::vec3 specularColor;

    SubMesh() : vao(nullptr), vbo(nullptr), ebo(nullptr), 
                diffuseMap(0), specularMap(0), normalMap(0),
                diffuseColor(0.8f, 0.8f, 0.8f), specularColor(0.2f, 0.2f, 0.2f) {}

    void setup() {
        vao = new VAO();
        vao->bind();
        vbo = new VBO((float*)geometry.G.data(), geometry.G.size() * sizeof(VertexData));
        ebo = new EBO(geometry.V);

        vao->linkAttrib(*vbo, 0, 3, GL_FLOAT, sizeof(VertexData), (void*)0);
        vao->linkAttrib(*vbo, 1, 3, GL_FLOAT, sizeof(VertexData), (void*)sizeof(Vec3));
        vao->linkAttrib(*vbo, 2, 2, GL_FLOAT, sizeof(VertexData), (void*)(2 * sizeof(Vec3)));

        vao->unbind();
    }

    void draw(Shader& shader) {
        if (diffuseMap != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, diffuseMap);
            shader.setBool("useDiffuseMap", true);
        } else {
            shader.setBool("useDiffuseMap", false);
            shader.setVec3("baseDiffuse", diffuseColor);
        }

        if (specularMap != 0) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, specularMap);
            shader.setBool("useSpecularMap", true);
        } else {
            shader.setBool("useSpecularMap", false);
            shader.setVec3("baseSpecular", specularColor);
        }

        if (normalMap != 0) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, normalMap);
            shader.setBool("useNormalMap", true);
        } else {
            shader.setBool("useNormalMap", false);
        }

        vao->bind();
        glDrawElements(GL_TRIANGLES, geometry.V.size(), GL_UNSIGNED_INT, 0);
        vao->unbind();
    }

    ~SubMesh() {
        if (vao) delete vao;
        if (vbo) delete vbo;
        if (ebo) delete ebo;
    }
};

class Model {
public:
    std::vector<SubMesh*> meshes;
    std::string directory;

    Model(const std::string& path) {
        loadModel(path);
    }

    void draw(Shader& shader) {
        for (auto mesh : meshes) {
            mesh->draw(shader);
        }
    }

    ~Model() {
        for (auto mesh : meshes) delete mesh;
    }

private:
    void loadModel(const std::string& path) {
        tinyobj::ObjReaderConfig reader_config;
        reader_config.mtl_search_path = path.substr(0, path.find_last_of('/')) + "/";
        directory = reader_config.mtl_search_path;

        tinyobj::ObjReader reader;

        if (!reader.ParseFromFile(path, reader_config)) {
            if (!reader.Error().empty()) {
                std::cerr << "TinyObjReader: " << reader.Error();
            }
            return;
        }

        if (!reader.Warning().empty()) {
            std::cout << "TinyObjReader: " << reader.Warning();
        }

        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();
        auto& materials = reader.GetMaterials();

        // One shape usually maps to one SubMesh.
        for (size_t s = 0; s < shapes.size(); s++) {
            SubMesh* submesh = new SubMesh();
            
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
                size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

                for (size_t v = 0; v < fv; v++) {
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                    VertexData vertex;
                    vertex.Position = {
                        attrib.vertices[3 * size_t(idx.vertex_index) + 0],
                        attrib.vertices[3 * size_t(idx.vertex_index) + 1],
                        attrib.vertices[3 * size_t(idx.vertex_index) + 2]
                    };

                    if (idx.normal_index >= 0) {
                        vertex.Normal = {
                            attrib.normals[3 * size_t(idx.normal_index) + 0],
                            attrib.normals[3 * size_t(idx.normal_index) + 1],
                            attrib.normals[3 * size_t(idx.normal_index) + 2]
                        };
                    } else {
                        vertex.Normal = {0.0f, 1.0f, 0.0f};
                    }

                    if (idx.texcoord_index >= 0) {
                        vertex.TexCoords = {
                            attrib.texcoords[2 * size_t(idx.texcoord_index) + 0],
                            attrib.texcoords[2 * size_t(idx.texcoord_index) + 1]
                        };
                    } else {
                        vertex.TexCoords = {0.0f, 0.0f};
                    }

                    submesh->geometry.G.push_back(vertex);
                    submesh->geometry.V.push_back(submesh->geometry.G.size() - 1);
                }
                index_offset += fv;
            }

            // Material
            int material_id = shapes[s].mesh.material_ids.empty() ? -1 : shapes[s].mesh.material_ids[0];
            if (material_id >= 0) {
                tinyobj::material_t mat = materials[material_id];
                if (!mat.diffuse_texname.empty())
                    submesh->diffuseMap = loadTexture((directory + mat.diffuse_texname).c_str());
                else 
                    submesh->diffuseColor = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
                
                if (!mat.specular_texname.empty())
                    submesh->specularMap = loadTexture((directory + mat.specular_texname).c_str());
                else
                    submesh->specularColor = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
                
                // Normal maps can be stored in normal_texname or bump_texname
                if (!mat.normal_texname.empty())
                    submesh->normalMap = loadTexture((directory + mat.normal_texname).c_str());
                else if (!mat.bump_texname.empty())
                    submesh->normalMap = loadTexture((directory + mat.bump_texname).c_str());
            }

            submesh->setup();
            meshes.push_back(submesh);
        }
    }
};

#endif
