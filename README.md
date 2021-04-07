# kc_truetypeassembler.h
Single header C library for assembling textured quads for rendering text. This library does not perform rendering directly and instead provides a vertex buffer of vertices (in either Screen Space or Clip Space) and texture coordinates. This allows the library to work seamlessly with both OpenGL and DirectX. Very lightweight: ~450 LOC. See https://github.com/kevinmkchin/opengl-3d-renderer for use examples. 

PURPOSE: Text rendering is a non-trivial task, and this library strives to make it easy and frictionless.

![](https://github.com/kevinmkchin/TrueTypeAssembler/blob/main/misc/console.gif?raw=true)

![](https://github.com/kevinmkchin/TrueTypeAssembler/blob/main/misc/text-buffer-assembly.gif?raw=true)

This library REQUIRES [Sean Barrett's stb_truetype.h](https://github.com/nothings/stb/blob/master/stb_truetype.h) library (which is also a single header). Include like so:

![](https://github.com/kevinmkchin/TrueTypeAssembler/blob/main/misc/include.png?raw=true)

Public domain.

```
 - kevinmkchin's TrueType Assembler -

    Do this:
        #define KC_TRUETYPEASSEMBLER_IMPLEMENTATION
    before you include this file in *one* C or C++ file to create the implementation.
        // i.e. it should look like this:
        #include <math.h>    // I believe stb_truetype.h requires math.h
        #include <stdlib.h>
        #include ...
        #include ...
        #define STB_TRUETYPE_IMPLEMENTATION
        #include "stb_truetype.h"
        #define KC_TRUETYPEASSEMBLER_IMPLEMENTATION
        #include "kc_truetypeassembler.h"

INTRO & PURPOSE:
    Single-header C library to generate textured quads for rendering text to the screen.
    This library does not perform rendering directly and instead provides a vertex buffer
    of vertices (in either Screen Space or Clip Space) and texture coordinates. This allows
    the library to work seamlessly with both OpenGL and DirectX. Probably also works with
    other graphics APIs out there...

    Text rendering is a non-trivial task, and this library strives to make it easy and frictionless.

    This library strives to solve 2 problems:
    - Creating an individual vertex buffer / textured quad for every single character you
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

    You can also make the library create and return an index buffer if you are using indexed
    drawing. You don't need to do this if you are not using indexed drawing.
    See the following links if you don't know what vertex indices or index buffers are:
    http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-9-vbo-indexing/
    https://www.learnopengles.com/tag/index-buffer-object/

    This library is not responsible for telling the graphics card to render text. You can
    do that on your own in your preferred graphics API using the vertex buffer (and index
    buffer if you enabled index drawing) that this library creates and a quad/ui rendering shader.
    
    > Vertex Coordinates:
        By default, Screen Space coordinates are generated instead of Clip Space coordinates.

        Quick reminder - Screen Space coordinates are where the top left corner of the
        window/screen is defined as x: 0, y: 0 and the bottom right corner is x: window width,
        y: window height. Clip Space coordinates are where the top left corner is x: -1, y: 1
        and the bottom right corner is x: 1, y: -1 (with 0, 0 being the center of the window).
        (https://learnopengl.com/Getting-started/Coordinate-Systems)

        You can pick which coordinate system to generate vertices for. If you already have an
        orthographic projection matrix for rendering UI or text, then you can make this library
        generate Screen Space coordinates and apply the orthographic projection matrix in your
        UI or text shader. You could also just create an orthographic projection matrix for this
        text-rendering purpose:
        https://glm.g-truc.net/0.9.1/api/a00237.html#ga71777a3b1d4fe1729cccf6eda05c8127
        https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glOrtho.xml

        If you just want to use Clip Space coordinates, then you need to set the flag for it:
            kctta_setflags(KCTTA_USE_CLIPSPACE_COORDS);
        and tell the library the window size:
            kctta_windowsize(WIDTH, HEIGHT);
        If you are using Screen Space coordinates, you don't need to do any of that.

    > Config Flags:
        Use the following flags with kctta_setflags(_flags_):
        KCTTA_CREATE_INDEX_BUFFER:
            Sets the libary to use indexed vertices and return a index buffer as well when
            grabbing buffers.
        KCTTA_USE_CLIPSPACE_COORDS:
            Sets the library to use generate vertices in Clip Space coordinates instead of
            Screen Space coordinates.
        KCTTA_NEWLINE_ABOVE:
            Sets the library to move the cursor above the current line instead of below
            when calling kctta_new_line.

    > By Default:
        - no indexed drawing (unless specified with flag KCTTA_CREATE_INDEX_BUFFER)
        - generates vertices in screenspace coordinates (unless specified with flag KCTTA_USE_CLIPSPACE_COORDS)
        - "next line" is the line below the line we are on (unless specified with flag KCTTA_NEWLINE_ABOVE)

    > The following "#define"s are unnecessary but optional:
    You can do #define KCTTA_MAX_CHAR_IN_BUFFER X (before including this library) where X is the 
    maximum number of characters you want to allow in the vertex buffer at once. By default this value
    is 800 characters. Consider your memory use when setting this value because the memory for the
    vertex buffer and index buffers are located in the .data segment of the program's alloted memory.
    Every character increases the combined size of the two buffers by 120 bytes (e.g. 800 characters
    allocates 800 * 120 = 96000 bytes in the .data segment of memory)
        e.g. #define KCTTA_MAX_CHAR_IN_BUFFER 500
             #define KC_TRUETYPEASSEMBLER_IMPLEMENTATION
             #include "kc_truetypeassembler.h"
    You can also #define KCTTA_ASCII_FROM X and #define KCTTA_ASCII_TO Y where X and Y are the start and
    end ASCII codepoints to collect the font data for. In other words, if X is the character 'a' and Y is
    the character 'z', then the library will only collect the font data for the ASCII characters from 'a'
    to 'b'. By default, the start and end codepoints are set to ' ' and '~'.
        e.g. #define KCTTA_ASCII_FROM 'a'
             #define KCTTA_ASCII_TO 'z'
             #define KC_TRUETYPEASSEMBLER_IMPLEMENTATION
             #include "kc_truetypeassembler.h"

    > Things to be aware of:
        - TTAFont is around ~4KB, so don't copy it around. Just declare it once and then pass around
          a POINTER to it instead of passing it around by value.

    > Some types:
        TTAVertexBuffer - see comment at definition - This is what you want to get back from this library
        TTABitmap - see comment at definition - basically data and pointer to bitmap image in memory
        TTAGlyph - see comment at definition - info about specific ASCII glyph
        TTAFont - see comment at definition - info about font - HUGE struct

EXAMPLE (Pseudocode):

Do only once:
    Optional:   kctta_setflags(KCTTA_CREATE_INDEX_BUFFER);                <-- Configures the library with given flags
                            |
                            V
    Optional:   kctta_windowsize(window width=1920, window height=1080);  <-- Only required if using KCTTA_USE_CLIPSPACE_COORDS
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


EXAMPLE (C code using OpenGL):

    #define TEXT_SIZE 30

    unsigned char* font_file;
    // Read the font file on disk (e.g. "arial.ttf") into a byte buffer in memory (e.g. font_file) using your own method
    // If you are using SDL, you can use SDL_RW. You could also use stdio.h's file operations (fopen, fread, fclose, etc.).
    
    kctta_setup(KCTTA_CREATE_INDEX_BUFFER|KCTTA_USE_CLIPSPACE_COORDS);     // kc_truetypeassembler setting
    kctta_windowsize(WIDTH, HEIGHT);

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
            glEnableVertexAttribArray(0); // vertices coordinates stream
            glVertexAttribPointer(0, 2, // 2 because vertex is 2 floats (x and y)
                                  GL_FLOAT, GL_FALSE,
                                  4*4,  // 4 stride (x y u v) * 4 bytes (size of float)
                                  0);
            glEnableVertexAttribArray(1); // texture coordinates stream
            glVertexAttribPointer(0, 2, // 2 because texture coord is 2 floats (u and v)
                                  GL_FLOAT, GL_FALSE,
                                  4*4,  // 4 stride (x y u v) * 4 bytes (size of float)
                                  2*4); // offset of 8 because two floats, x y, come before u v
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,       // for both the target and source format, we can put GL_RED
                     font_handle.font_atlas.width,   // this just means the bit depth is 1 byte (just the alpha)
                     font_handle.font_atlas.height, 
                     0, GL_RED, GL_UNSIGNED_BYTE, 
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
