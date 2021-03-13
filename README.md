# kc_truetypeassembler.h
Single header C library for truetype text textured quad vertices assembly. It generates a vertices and texture coordinates array for creating vertex buffers to render text onto the screen. Works seamlessly with both OpenGL and DirectX. Very lightweight: ~450 LOC.

![](https://github.com/kevinmkchin/TrueTypeAssembler/blob/main/misc/text-buffer-assembly.gif?raw=true)

This library REQUIRES [Sean Barrett's stb_truetype.h](https://github.com/nothings/stb/blob/master/stb_truetype.h) library (which is also a single header). Include like so:

![](https://github.com/kevinmkchin/TrueTypeAssembler/blob/main/misc/includes.png?raw=true)

```
kc_truetypeassembler.h 

 - kevinmkchin's TrueType Assembler -

PURPOSE:
    Single-header library to generate vertices and texture coordinates array for
    creating Vertex Buffers to render text onto the screen. Works seamlessly with 
    both OpenGL and DirectX. Probably also works with other graphics APIs out there...

    This library strives to solve 2 problems:
    - Creating an individual vertex array / textured quad for every single character you
    want to draw is extremely inefficient - especially if each character has their own
    texture that needs to be binded to the graphics card.
    - Every character/glyph has varying sizes and parameters that affect how they should
    be drawn relative to all the other characters/glyphs. These must be considered when
    drawing a line of text.

    This library solves these problems by 
        1.  generating a single, large texture combining all the individual textures of the
            characters - called a font Texture Atlas
        2.  generating a single vertex buffer containing the vertices of all the quads 
            (https://en.wikipedia.org/wiki/Quadrilateral - one quad can draw one character) 
            required by the text you want to draw
        3.  handling the assembly of the quad vertices for the single vertex buffer by
            using the character/glyph metrics (size, offsets, etc.)

CONCEPT:
    This library serves as a text drawing "canvas". 
    You can "append" lines of text or characters to the "canvas". You can control where
    the text gets "appended" to the "canvas" by moving the "cursor" (setting its location
    on the screen). Then when you want to draw the "canvas" with your graphics API (OpenGL, 
    DirectX, etc.), you can "grab" the "canvas" (vertex buffer).
    Then, it is up to you and your graphics API to create the VAO and VBO on the graphics card
    using the vertex buffer you "grabbed" from this library.
    Check the USAGE EXAMPLEs below to see how this would translate to code.

USAGE:
    This library REQUIRES Sean Barrett's stb_truetype.h to be included beforehand:
    https://raw.githubusercontent.com/nothings/stb/master/stb_truetype.h

    This library's purpose is to return an array of vertices and texture coordinates:
        [ x, y, u, v, x, y, u, v, ......, x, y, u, v ]
    You can feed this into a Vertex Buffer with a Stride of 4.

    Since this library only returns an array of vertex and texture coordinates, you 
    should be able to feed that array into the vertex buffer of any graphics API and 
    get it working.

    This library is not responsible for rendering text. You can do that on your own in your
    preferred graphics API, a quad/ui rendering shader, and an orthogonal projection matrix.
    (https://www.learnopengles.com/tag/index-buffer-object/)

USAGE EXAMPLE (Pseudocode):

Do only once:
    Optional:   kctta_use_index_buffer(b_use = 1);                        <-- Set this to true if you are using indexed draws
                            |
                            V
    Required:   kctta_init_font(font_handle, font_buffer, font_height);   <-- DO ONLY ONCE PER FONT (or per font resolution)

Loop:
    Optional:   kctta_move_cursor(x = 640, y = 360);                      <-- Set the cursor to x y (where to start drawing)
                            |
                            V
    Required:   kctta_append_line("some text", font_handle, font_size);   <-- text to draw
                            |
                            V
    Optional:   kctta_new_line(x = 640, font);                            <-- Go to next line, set cursor x to 640
                            |
                            V
    Optional:   kctta_append_line("next lin", font_handle, font_size);    <-- next line to draw
                            |
                            V
    Optional:   kctta_append_glyph('e', font_handle, font_size);          <-- Can append individual glyphs also
                            |
                            V
    Required:   TTAVertexBuffer %grabbedbuffer% = kctta_grab_buffer();    <-- Grabs the buffer
                            |
                            V
    Required:   * create/bind Vertex Array Object and Vertex Buffer Objects on GPU using your graphics API and %grabbedbuffer%
                            |
                            V
    Optional:   kctta_clear_buffer();           <-- REQUIRED IF you want to clear the appended text to append NEW text
                                                    Only clear AFTER you bind the vertex and index buffers of %grabbedbuffer%
                                                    to the VAO and VBO on the GPU.


USAGE EXAMPLE (C code using OpenGL):

    #define TEXT_SIZE 30

    unsigned char* font_file;
    // Read the font file on disk (e.g. "arial.ttf") into a byte buffer in memory (e.g. font_file) using your own method
    // If you are using SDL, you can use SDL_RW. You could also use stdio.h's file operations (fopen, fread, fclose, etc.).
    
    kctta_use_index_buffer(1); // Enable this if you are using Indexed Drawing
    TTAFont font_handle;
    kctta_init_font(&font_handle, font_file, TEXT_SIZE);
    // TEXT_SIZE in init_font doesn't need to be the same as the TEXT_SIZE we use when we call append_line or glyph
    // Now font_handle has all the info we need.

    kctta_clear_buffer();
    kctta_move_cursor(100, 100);
    kctta_append_line("Hello, world!", &font_handle);

    TTAVertexBuffer vb = kctta_grab_buffer();

    // That's it. That's all you need to interact with this library. Everything below is just
    // using the vertex buffer from the library to actually get the text drawing in OpenGL.

    GLuint VAO_ID, VBO_ID, IBO_ID, TextureID;
    // Creating the VAO for our text in the GPU memory
    glBindVertexArray(VAO_ID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_ID);
            glBufferData(GL_ARRAY_BUFFER,
                         vb.vertices_array_count * 4,
                         vb.vertex_buffer,
                         GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_ID);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         vb.indices_array_count * 4,
                         vb.index_buffer,
                         GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    // Creating the font texture in GPU memory
    glGenTextures(1, &TextureID);     
    glBindTexture(GL_TEXTURE_2D, TextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 
                     font_handle.font_atlas.width,
                     font_handle.font_atlas.height, 
                     0, source_format, GL_UNSIGNED_BYTE, 
                     font_handle.font_atlas.pixels);
    glActiveTexture(GL_TEXTURE0);
    // Draw call
    glUseProgram(SHADER PROGRAM FOR TEXT); // The text shader should use an orthographic projection matrix
    glBindVertexArray(mesh.id_vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.id_ibo);
            glDrawElements(GL_TRIANGLES, vb.indices_array_count, GL_UNSIGNED_INT, nullptr);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);
```
