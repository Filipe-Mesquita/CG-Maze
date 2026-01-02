#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <map>
#include <string>
#include <glm/glm.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

class TextRenderer {
public:
    // Holds a list of pre-compiled Characters
    std::map<char, Character> Characters;
    
    // Constructor
    TextRenderer();
    
    // Pre-compiles a list of characters from the given font
    void Load(std::string font, unsigned int fontSize);
    
    // Renders a string of text using the precompiled list of characters
    void RenderText(std::string text, float x, float y, float scale, glm::vec3 color = glm::vec3(1.0f));
    
private:
    // Render state
    unsigned int VAO, VBO;
};

#endif