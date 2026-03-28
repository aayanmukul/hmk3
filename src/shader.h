#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class Shader
{
public:
    GLuint id { 0 };

    Shader(const char* vertPath, const char* fragPath)
    {
        std::string vertSrc = readFile(vertPath);
        std::string fragSrc = readFile(fragPath);

        GLuint vs = compile(GL_VERTEX_SHADER,   vertSrc.c_str(), vertPath);
        GLuint fs = compile(GL_FRAGMENT_SHADER, fragSrc.c_str(), fragPath);

        id = glCreateProgram();
        glAttachShader(id, vs);
        glAttachShader(id, fs);
        glLinkProgram(id);
        checkLink(id);

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    void use() const { glUseProgram(id); }

    void setBool (const std::string& n, bool  v) const { glUniform1i(loc(n), (int)v); }
    void setInt  (const std::string& n, int   v) const { glUniform1i(loc(n), v); }
    void setFloat(const std::string& n, float v) const { glUniform1f(loc(n), v); }

    void setVec3(const std::string& n, const glm::vec3& v) const
        { glUniform3fv(loc(n), 1, glm::value_ptr(v)); }

    void setVec3(const std::string& n, float x, float y, float z) const
        { glUniform3f(loc(n), x, y, z); }

    void setMat4(const std::string& n, const glm::mat4& m) const
        { glUniformMatrix4fv(loc(n), 1, GL_FALSE, glm::value_ptr(m)); }

private:
    GLint loc(const std::string& n) const
        { return glGetUniformLocation(id, n.c_str()); }

    static std::string readFile(const char* path)
    {
        std::ifstream f(path);
        if (!f.is_open()) {
            std::cerr << "[Shader] Cannot open: " << path << "\n";
            return {};
        }
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    static GLuint compile(GLenum type, const char* src, const char* label)
    {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);

        GLint ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetShaderInfoLog(s, sizeof(log), nullptr, log);
            std::cerr << "[Shader] Compile error (" << label << "):\n" << log << "\n";
        }
        return s;
    }

    static void checkLink(GLuint prog)
    {
        GLint ok;
        glGetProgramiv(prog, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
            std::cerr << "[Shader] Link error:\n" << log << "\n";
        }
    }
};
