/*

vertext.h

                                    .                             .
                                 .o8                           .o8
oooo    ooo  .ooooo.  oooo d8b .o888oo  .ooooo.  oooo    ooo .o888oo
 `88.  .8'  d88' `88b `888""8P   888   d88' `88b  `88b..8P'    888
  `88..8'   888ooo888  888       888   888ooo888    Y888'      888
   `888'    888    .o  888       888 . 888    .o  .o8"'88b     888 .
    `8'     `Y8bod8P' d888b      "888" `Y8bod8P' o88'   888o   "888"



By Kevin Chin 2022 (https://kevch.in/)

    Requires stb_truetype.h (https://github.com/nothings/stb/blob/master/stb_truetype.h)
    Do this:
        #define VERTEXT_IMPLEMENTATION
    before you include this file in *one* C or C++ file to create the implementation.
        // i.e. it should look like this:
        #include ...
        #include ...
        #define STB_TRUETYPE_IMPLEMENTATION
        #include "stb_truetype.h"
        #define VERTEXT_IMPLEMENTATION
        #include "vertext.h"

INTRO & PURPOSE:
    Single-header C library to generate textured quads for rendering text to the screen.
    This library does not perform rendering directly and instead provides a vertex buffer
    of vertices (in either Screen Space or Clip Space) and texture coordinates. This allows
    the library to work seamlessly with both OpenGL and DirectX. Probably also works with
    other graphics APIs too...

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
            vtxt_setflags(VTXT_USE_CLIPSPACE_COORDS);
        and tell the library the window size:
            vtxt_backbuffersize(WIDTH, HEIGHT);
        If you are using Screen Space coordinates, you don't need to do any of that.

    > Config Flags:
        Use the following flags with vtxt_setflags(_flags_):
        VTXT_CREATE_INDEX_BUFFER:
            Sets the libary to use indexed vertices and return a index buffer as well when
            grabbing buffers.
        VTXT_USE_CLIPSPACE_COORDS:
            Sets the library to use generate vertices in Clip Space coordinates instead of
            Screen Space coordinates.
        VTXT_NEWLINE_ABOVE:
            Sets the library to move the cursor above the current line instead of below
            when calling vtxt_new_line.
        VTXT_FLIP_Y:
            Flips y vertex position in case you are using a coordinate system where "up" is negative y.

    > By Default:
        - no indexed drawing (unless specified with flag VTXT_CREATE_INDEX_BUFFER)
        - generates vertices in screenspace coordinates (unless specified with flag VTXT_USE_CLIPSPACE_COORDS)
        - "next line" is the line below the line we are on (unless specified with flag VTXT_NEWLINE_ABOVE)

    > The following "#define"s are unnecessary but optional:
        #define VTXT_MAX_CHAR_IN_BUFFER X (before including this library) where X is the
        maximum number of characters you want to allow in the vertex buffer at once. By default this value
        is 800 characters. Consider your memory use when setting this value because the memory for the
        vertex buffer and index buffers are located in the .data segment of the program's alloted memory.
        Every character increases the combined size of the two buffers by 120 bytes (e.g. 800 characters
        allocates 800 * 120 = 96000 bytes in the .data segment of memory)
            e.g. #define VTXT_MAX_CHAR_IN_BUFFER 500
                 #define VERTEXT_IMPLEMENTATION
                 #include "vertext.h"

        #define VTXT_ASCII_FROM X and #define VTXT_ASCII_TO Y where X and Y are the start and
        end ASCII codepoints to collect the font data for. In other words, if X is the character 'a' and Y is
        the character 'z', then the library will only collect the font data for the ASCII characters from 'a'
        to 'z'. By default, the start and end codepoints are set to ' ' and '~'.
            e.g. #define VTXT_ASCII_FROM 'a'
                 #define VTXT_ASCII_TO 'z'
                 #define VERTEXT_IMPLEMENTATION
                 #include "vertext.h"

        #define VTXT_STATIC to make function declarations and function definitions static. This makes
        the implementation private to the source file that creates it. This allows you to have multiple
        instances of this library in your project without collision. You could use multiple vertex
        buffers at the same time without clearing the buffers.

    > Things to be aware of:
        - vtxt_font is around ~4KB, so don't copy it around. Just declare it once and then pass around
          a POINTER to it instead of passing it around by value.

    > Some types:
        vtxt_vertex_buffer - see comment at definition - This is what you want to get back from this library
        vtxt_bitmap - see comment at definition - basically data and pointer to bitmap image in memory
        vtxt_glyph - see comment at definition - info about specific ASCII glyph
        vtxt_font - see comment at definition - info about font - HUGE struct

EXAMPLE (Pseudocode):

Do only once:
    Optional:   vtxt_setflags(VTXT_CREATE_INDEX_BUFFER);                <-- Configures the library with given flags
                            |
                            V
    Optional:   vtxt_backbuffersize(window width=1920, window height=1080);  <-- Only required if using VTXT_USE_CLIPSPACE_COORDS
                            |
                            V
    Required:   vtxt_init_font(font_handle, font_buffer, font_height_px);   <-- DO ONLY ONCE PER FONT (or per font resolution)

Loop:
    Optional:   vtxt_move_cursor(x = 640, y = 360);                      <-- Set the cursor to x y (where to start drawing)
                            |
                            V
    Required:   vtxt_append_line("some text", font_handle, text_height_px);   <-- text to draw
                            |
                            V
    Optional:   vtxt_new_line(x = 640, font);                            <-- Go to next line, set cursor x to 640
                            |
                            V
    Optional:   vtxt_append_line("next lin", font_handle, text_height_px);    <-- next line to draw
                            |
                            V
    Optional:   vtxt_append_glyph('e', font_handle, text_height_px);          <-- Can append individual glyphs also
                            |
                            V
    Required:   vtxt_vertex_buffer grabbedBuffer = vtxt_grab_buffer();    <-- Grabs the buffer
                            |
                            V
    Required:   * create/bind Vertex Array Object and Vertex Buffer Objects on GPU using your graphics API and grabbedBuffer
                            |
                            V
    Optional:   vtxt_clear_buffer();           <-- REQUIRED IF you want to clear the appended text to append NEW text
                                                    Only clear AFTER you bind the vertex and index buffers of grabbedBuffer
                                                    to the VAO and VBO on the GPU.


EXAMPLE (C code using OpenGL):

    #define TEXT_SIZE 30

    unsigned char* font_file;
    // Read the font file on disk (e.g. "arial.ttf") into a byte buffer in memory (e.g. font_file) using your own method
    // If you are using SDL, you can use SDL_RW. You could also use stdio.h's file operations (fopen, fread, fclose, etc.).
    
    vtxt_setflags(VTXT_CREATE_INDEX_BUFFER|VTXT_USE_CLIPSPACE_COORDS);     // vertext.h setting
    vtxt_backbuffersize(WIDTH, HEIGHT);

    vtxt_font font_handle;
    vtxt_init_font(&font_handle, font_file, TEXT_SIZE);
    // TEXT_SIZE in init_font doesn't need to be the same as the TEXT_SIZE we use when we call append_line or glyph
    // Now font_handle has all the info we need.

    vtxt_clear_buffer();
    vtxt_move_cursor(100, 100);
    vtxt_append_line("Hello, world!", &font_handle);

    vtxt_vertex_buffer vb = vtxt_grab_buffer();

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

TODO:
    - Signed Distance Fields
    - Kerning
    - Top-to-bottom text (vertical text)
    - Add flag to choose winding order (currently counter-clockwise winding order)
*/
#ifndef _INCLUDE_VERTEXT_H_
#define _INCLUDE_VERTEXT_H_

#ifndef VTXT_ASCII_FROM
#define VTXT_ASCII_FROM ' '    // starting ASCII codepoint to collect font data for
#endif
#ifndef VTXT_ASCII_TO
#define VTXT_ASCII_TO '~'      // ending ASCII codepoint to collect font data for
#endif
#define VTXT_GLYPH_COUNT VTXT_ASCII_TO - VTXT_ASCII_FROM + 1

#ifdef VTXT_STATIC
#define VTXT_DEF static
#else
#define VTXT_DEF extern
#endif

/** Stores a pointer to the vertex buffer assembly array and the count of vertices in the 
    array (total length of array would be count of vertices * 4).
*/
typedef struct vtxt_vertex_buffer
{
    int             vertex_count;           // count of vertices in vertex buffer array (4 elements per vertex, so vertices_array_count / 4 = vertex_count)
    int             vertices_array_count;   // count of elements in vertex buffer array
    int             indices_array_count;    // count of elements in index buffer array
    float*          vertex_buffer;          // pointer to vertex buffer array
    unsigned int*   index_buffer;           // pointer to index buffer array
} vtxt_vertex_buffer;

/** vtxt_bitmap is a handle to hold a pointer to an unsigned byte bitmap in memory. Length/count
    of bitmap elements = width * height.
*/
typedef struct vtxt_bitmap
{
    int             width;      // bitmap width
    int             height;     // bitmap height
    unsigned char*  pixels;     // unsigned byte bitmap
} vtxt_bitmap;

/** Stores information about a glyph. The glyph is identified via codepoint (ASCII).
    Check out https://learnopengl.com/img/in-practice/glyph.png 
    Learn more about glyph metrics if you want.
*/
typedef struct vtxt_glyph
{
    float           width, height, advance,offset_x,offset_y,min_u,min_v,max_u,max_v;
    char            codepoint;
} vtxt_glyph;

/** vtxt_font is a handle to hold font information. It's around ~4KB, so don't copy it around all the time.
*/
typedef struct vtxt_font
{
    int             font_height_px;             // how tall the font's vertical extent is in pixels
    float           ascender;                   // https://en.wikipedia.org/wiki/Ascender_(typography)
    float           descender;                  // https://en.wikipedia.org/wiki/Descender
    float           linegap;                    // gap between the bottom of the descender of one line to the top of the ascender of the line below
    vtxt_bitmap     font_atlas;                 // stores the bitmap for the font texture atlas (https://en.wikipedia.org/wiki/Texture_atlas#/media/File:Texture_Atlas.png)
    vtxt_glyph      glyphs[VTXT_GLYPH_COUNT];   // array for glyphs information
} vtxt_font;

enum _vtxt_config_flags_t
{
    VTXT_CREATE_INDEX_BUFFER     = 1 << 0,
    VTXT_USE_CLIPSPACE_COORDS    = 1 << 1,
    VTXT_NEWLINE_ABOVE           = 1 << 2,
    VTXT_FLIP_Y                  = 1 << 3,
};

/** Configures this library to use the settings defined by _vtxt_config_flags_t.
    e.g. vtxt_setflags(VTXT_CREATE_INDEX_BUFFER | VTXT_USE_CLIPSPACE_COORDS);
*/
VTXT_DEF void vtxt_setflags(int newconfig);

/** ONLY IF YOU WANT CLIPSPACE COORDINATES INSTEAD OF SCREENSPACE COORDINATES
    Tells vertext.h the size of your application's backbuffer size. If the
    backbuffer size changes, then you should call this again to update the size.
*/
VTXT_DEF void vtxt_backbuffersize(int width, int height);

/** Initializes a vtxt_font font handle to store glyphs information, font information, and the font texture atlas.
    Collects the glyphs and font information from the font file (given as a binary buffer in memory)
    and generates the font texture atlas (https://en.wikipedia.org/wiki/Texture_atlas#/media/File:Texture_Atlas.png)
    Expensive, so you should only do this ONCE per font (and font size) and just keep the vtxt_font around somewhere.
    font_height_in_pixels := How tall the font's vertical extent (above the baseline) should be in pixels
*/
VTXT_DEF void vtxt_init_font(vtxt_font*       font_handle,
                             unsigned char*   font_buffer,
                             int              font_height_in_pixels);

/** Move cursor location (cursor represents the position on the screen where text is placed)
*/
VTXT_DEF void vtxt_move_cursor(int x,
                               int y);

/** Go to new line and set X location of cursor
*/
VTXT_DEF void vtxt_new_line(int          x,
                            vtxt_font*   font,
                            int          text_height_px);

/** Assemble quads for a line of text and append to vertex buffer.
    line_of_text is the text you want to draw e.g. "some text I want to Draw".
    font is the vtxt_font font handle that contains the font you want to use.
    text_height_px is the desired text height in pixels.
*/
VTXT_DEF void vtxt_append_line(const char*   line_of_text,
                               vtxt_font*    font,
                               int           text_height_px);

/** Same as vtxt_append_line but center horizontally where the cursor is. */
VTXT_DEF void vtxt_append_line_centered(const char* line_of_text,
                                        vtxt_font*  font,
                                        int         text_height_px);

/** Same as vtxt_append_line but with text using right-alignment (the text is to the left of the cursor). */
VTXT_DEF void vtxt_append_line_align_right(const char* line_of_text, 
                                           vtxt_font*  font, 
                                           int         text_height_px);

/** Assemble quad for a glyph and append to vertex buffer.
    font is the vtxt_font font handle that contains the font you want to use.
    text_height_px is the maximum vertical extent of the glyph in pixels
*/
VTXT_DEF void vtxt_append_glyph(const char   in_glyph,
                                vtxt_font*   font,
                                int          text_height_px);

/** Get vtxt_vertex_buffer with a pointer to the vertex buffer array
    and vertex buffer information.
*/
VTXT_DEF vtxt_vertex_buffer vtxt_grab_buffer();

/** Call before starting to append new text.
    Clears the vertex buffer that text is being appended to. 
    If you called vtxt_grab_buffer and want to use the buffer you received,
    make sure you pass the buffer to OpenGL (glBufferData) or make a copy of
    the buffer before calling vtxt_clear_buffer.
*/
VTXT_DEF void vtxt_clear_buffer();

/** Set an offset to font linegap. Default is 0. */
VTXT_DEF void vtxt_set_linegap_offset(float offset);

/** Returns the width and height of the minimum bounding box containing
    the given text using the given font and text height. */
VTXT_DEF void vtxt_get_text_bounding_box_info(float* width_out,
                                              float* height_out,
                                              const char* text,
                                              vtxt_font* font,
                                              int text_height_px);


#endif // _INCLUDE_VERTEXT_H_


///////////////////// IMPLEMENTATION //////////////////////////
#ifdef VERTEXT_IMPLEMENTATION

#define _vtxt_internal static      // vtxt local static variable
#ifndef VTXT_MAX_CHAR_IN_BUFFER
#define VTXT_MAX_CHAR_IN_BUFFER 800    // maximum characters allowed in vertex buffer ("canvas")
#endif
#define VTXT_MAX_FONT_RESOLUTION 100   // maximum font resolution when initializing font
#define VTXT_DESIRED_ATLAS_WIDTH 400   // width of the font atlas
#define VTXT_ATLAS_PAD_X 1                // x padding between the glyph textures on the texture atlas
#define VTXT_ATLAS_PAD_Y 1                // y padding between the glyph textures on the texture atlas

#define _vtxt_ceil(num) ((num) == (float)((int)(num)) ? (int)(num) : (((int)(num)) + 1))

// Buffers for vertices and texture_coords before they are written to GPU memory.
// If you have a pointer to these buffers, DO NOT let these buffers be overwritten
// before you bind the data to GPU memory.
_vtxt_internal float _vtxt_vertex_buffer[VTXT_MAX_CHAR_IN_BUFFER * 6 * 4]; // 800 characters * 6 vertices * (2 xy + 2 uv)
_vtxt_internal int _vtxt_vertex_count = 0; // Each vertex takes up 4 places in the assembly_buffer
_vtxt_internal unsigned int _vtxt_index_buffer[VTXT_MAX_CHAR_IN_BUFFER * 6];
_vtxt_internal int _vtxt_index_count = 0;
_vtxt_internal int _vtxt_config = 0b0;
_vtxt_internal float _vtxt_linegap_offset = 0.f;
_vtxt_internal int _vtxt_cursor_x = 0;   // top left of the screen is pixel (0, 0), bot right of the screen is pixel (screen buffer width, screen buffer height)
_vtxt_internal int _vtxt_cursor_y = 100; // cursor points to the base line at which to start drawing the glyph
_vtxt_internal int _vtxt_screen_w_for_clipspace = 800;
_vtxt_internal int _vtxt_screen_h_for_clipspace = 600;

VTXT_DEF void
vtxt_setflags(int newconfig)
{
    int oldconfig = _vtxt_config;
    _vtxt_config = newconfig;

    if((oldconfig & VTXT_CREATE_INDEX_BUFFER) != (_vtxt_config & VTXT_CREATE_INDEX_BUFFER))
    {
        vtxt_clear_buffer();
    }
}

VTXT_DEF void
vtxt_set_linegap_offset(float offset)
{
    _vtxt_linegap_offset = offset;
}

VTXT_DEF void
vtxt_backbuffersize(int width, int height)
{
    _vtxt_screen_w_for_clipspace = width;
    _vtxt_screen_h_for_clipspace = height;
}

VTXT_DEF void
vtxt_init_font(vtxt_font* font_handle, unsigned char* font_buffer, int font_height_in_pixels)
{
    int desired_atlas_width = VTXT_DESIRED_ATLAS_WIDTH;

    if(font_height_in_pixels > VTXT_MAX_FONT_RESOLUTION)
    {
        return;
    }

    // Font metrics
    stbtt_fontinfo stb_font_info;
    stbtt_InitFont(&stb_font_info, font_buffer, 0);
    float stb_scale = stbtt_ScaleForMappingEmToPixels(&stb_font_info, (float)font_height_in_pixels);
    int stb_ascender;
    int stb_descender;
    int stb_linegap;
    stbtt_GetFontVMetrics(&stb_font_info, &stb_ascender, &stb_descender, &stb_linegap);
    font_handle->font_height_px = font_height_in_pixels;
    font_handle->ascender = (float)stb_ascender * stb_scale;
    font_handle->descender = (float)stb_descender * stb_scale;
    font_handle->linegap = (float)stb_linegap * stb_scale;

    // LOAD GLYPH BITMAP AND INFO FOR EVERY CHARACTER WE WANT IN THE FONT
    vtxt_bitmap temp_glyph_bitmaps[VTXT_GLYPH_COUNT];
    int tallest_glyph_height = 0;
    int aggregate_glyph_width = 0;
    // load glyph data
    for(char char_index = VTXT_ASCII_FROM; char_index <= VTXT_ASCII_TO; ++char_index) // ASCII
    {
        vtxt_glyph glyph;
        
        // get glyph metrics from stbtt
        int stb_advance;
        int stb_leftbearing;
        stbtt_GetCodepointHMetrics(&stb_font_info, 
                                   char_index, 
                                   &stb_advance, 
                                   &stb_leftbearing);
        glyph.codepoint = char_index;
        glyph.advance = (float)stb_advance * stb_scale;
        int stb_width, stb_height;
        int stb_offset_x, stb_offset_y;
        unsigned char* stb_bitmap_temp = stbtt_GetCodepointBitmap(&stb_font_info,
                                                                0, stb_scale,
                                                                char_index,
                                                                &stb_width,
                                                                &stb_height,
                                                                &stb_offset_x,
                                                                &stb_offset_y);
        glyph.width = (float)stb_width;
        glyph.height = (float)stb_height;
        glyph.offset_x = (float)stb_offset_x;
        glyph.offset_y = (float)stb_offset_y;

        // Copy stb_bitmap_temp bitmap into glyph's pixels bitmap so we can free stb_bitmap_temp
        int iter = char_index - VTXT_ASCII_FROM;
        temp_glyph_bitmaps[iter].pixels = (unsigned char*) calloc((size_t)glyph.width * (size_t)glyph.height, 1);
        for(int row = 0; row < (int) glyph.height; ++row)
        {
            for(int col = 0; col < (int) glyph.width; ++col)
            {
                // Flip the bitmap image from top to bottom to bottom to top
                temp_glyph_bitmaps[iter].pixels[row * (int) glyph.width + col] = stb_bitmap_temp[((int) glyph.height - row - 1) * (int) glyph.width + col];
            }
        }
        temp_glyph_bitmaps[iter].width = (int) glyph.width;
        temp_glyph_bitmaps[iter].height = (int) glyph.height;
        aggregate_glyph_width += (int)glyph.width + VTXT_ATLAS_PAD_X;
        if(tallest_glyph_height < (int)glyph.height)
        {
            tallest_glyph_height = (int)glyph.height;
        }
        stbtt_FreeBitmap(stb_bitmap_temp, 0);

        font_handle->glyphs[iter] = glyph;
    }

    int desired_atlas_height = (tallest_glyph_height + VTXT_ATLAS_PAD_Y)
                               * _vtxt_ceil((float)aggregate_glyph_width / (float)desired_atlas_width);
    // Build font atlas bitmap based on these parameters
    vtxt_bitmap atlas;
    atlas.pixels = (unsigned char*) calloc(desired_atlas_width * desired_atlas_height, 1); // TODO avoid calloc here
    atlas.width = desired_atlas_width;
    atlas.height = desired_atlas_height;
    // COMBINE ALL GLYPH BITMAPS INTO FONT ATLAS
    int glyph_count = VTXT_GLYPH_COUNT;
    int atlas_x = 0;
    int atlas_y = 0;
    for(int i = 0; i < glyph_count; ++i)
    {
        vtxt_bitmap glyph_bitmap = temp_glyph_bitmaps[i];
        if (atlas_x + glyph_bitmap.width > atlas.width) // check if move atlas bitmap cursor to next line
        {
            atlas_x = 0;
            atlas_y += tallest_glyph_height + VTXT_ATLAS_PAD_Y;
        }

        for(int glyph_y = 0; glyph_y < glyph_bitmap.height; ++glyph_y)
        {
            for(int glyph_x = 0; glyph_x < glyph_bitmap.width; ++glyph_x)
            {
                atlas.pixels[(atlas_y + glyph_y) * atlas.width + atlas_x + glyph_x]
                    = glyph_bitmap.pixels[glyph_y * glyph_bitmap.width + glyph_x];
            }
        }
        font_handle->glyphs[i].min_u = (float) atlas_x / (float) atlas.width;
        font_handle->glyphs[i].min_v = (float) atlas_y / (float) atlas.height;
        font_handle->glyphs[i].max_u = (float) (atlas_x + glyph_bitmap.width) / (float) atlas.width;
        font_handle->glyphs[i].max_v = (float) (atlas_y + glyph_bitmap.height) / (float) atlas.height;

        atlas_x += glyph_bitmap.width + VTXT_ATLAS_PAD_X; // move the atlas bitmap cursor by glyph bitmap width

        free(glyph_bitmap.pixels);
    }
    font_handle->font_atlas = atlas;
}

VTXT_DEF void
vtxt_move_cursor(int x, int y)
{
    _vtxt_cursor_x = x;
    _vtxt_cursor_y = y;
}

VTXT_DEF void
vtxt_new_line(int x, vtxt_font* font, int text_height_px)
{
    float scale = (float)text_height_px / (float)font->font_height_px;
    float linegap = font->linegap + _vtxt_linegap_offset;
    _vtxt_cursor_x = x;
    if(_vtxt_config & VTXT_NEWLINE_ABOVE)
    {
        if(_vtxt_config & VTXT_FLIP_Y)
        {
            _vtxt_cursor_y += (int) ((-font->descender + linegap + font->ascender)*scale);
        }
        else
        {
            _vtxt_cursor_y -= (int) ((-font->descender + linegap + font->ascender)*scale);
        }
    }
    else
    {
        if(_vtxt_config & VTXT_FLIP_Y)
        {
            _vtxt_cursor_y -= (int) ((-font->descender + linegap + font->ascender)*scale);
        }
        else
        {
            _vtxt_cursor_y += (int) ((-font->descender + linegap + font->ascender)*scale);
        }
    }
}

VTXT_DEF void
__private_vtxt_append_glyph(const char in_glyph, vtxt_font* font, int text_height_px, float x_offset_from_cursor)
{
    if(in_glyph < VTXT_ASCII_FROM || in_glyph > VTXT_ASCII_TO) // Make sure we have the data for this glyph
    {
        return;
    }

    if(VTXT_MAX_CHAR_IN_BUFFER * 6 < _vtxt_vertex_count + 6) // Make sure we are not exceeding the array size
    {
        return;
    }

    float scale = (float)text_height_px / (float)font->font_height_px;
    vtxt_glyph glyph = font->glyphs[in_glyph - VTXT_ASCII_FROM];
    glyph.advance *= scale;
    glyph.width *= scale; // NOTE(Kevin): 2022-06-15 scale was float, but width and height were integers so rounding was causing text to render strangely - fixed by just changing width and height to floats
    glyph.height *= scale;
    glyph.offset_x *= scale;
    glyph.offset_y *= scale;

    // For each of the 6 vertices, fill in the _vtxt_vertex_buffer in the order x y u v
    int STRIDE = 4;

    float top = _vtxt_cursor_y + glyph.offset_y;
    float bot = _vtxt_cursor_y + glyph.offset_y + glyph.height;
    float left = _vtxt_cursor_x + glyph.offset_x + x_offset_from_cursor;
    float right = _vtxt_cursor_x + glyph.offset_x + glyph.width + x_offset_from_cursor;
    if(_vtxt_config & VTXT_FLIP_Y)
    {
        top = _vtxt_cursor_y - glyph.offset_y;
        bot = _vtxt_cursor_y - glyph.offset_y - glyph.height;
    }

    if(_vtxt_config & VTXT_USE_CLIPSPACE_COORDS)
    {
        top = (1.f - ((top / _vtxt_screen_h_for_clipspace) * 2.f));
        bot = (1.f - ((bot / _vtxt_screen_h_for_clipspace) * 2.f));
        left = ((left / _vtxt_screen_w_for_clipspace) * 2.f) - 1.f;
        right = ((right / _vtxt_screen_w_for_clipspace) * 2.f) - 1.f;
    }

    if(_vtxt_config & VTXT_CREATE_INDEX_BUFFER)
    {
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 0] = left;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 1] = bot;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 2] = glyph.min_u;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 3] = glyph.min_v;

        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 4] = left;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 5] = top;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 6] = glyph.min_u;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 7] = glyph.max_v;

        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 8] = right;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 9] = top;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 10] = glyph.max_u;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 11] = glyph.max_v;

        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 12] = right;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 13] = bot;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 14] = glyph.max_u;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 15] = glyph.min_v;

        _vtxt_index_buffer[_vtxt_index_count + 0] = _vtxt_vertex_count + 0;
        _vtxt_index_buffer[_vtxt_index_count + 1] = _vtxt_vertex_count + 2;
        _vtxt_index_buffer[_vtxt_index_count + 2] = _vtxt_vertex_count + 1;
        _vtxt_index_buffer[_vtxt_index_count + 3] = _vtxt_vertex_count + 0;
        _vtxt_index_buffer[_vtxt_index_count + 4] = _vtxt_vertex_count + 3;
        _vtxt_index_buffer[_vtxt_index_count + 5] = _vtxt_vertex_count + 2;

        _vtxt_vertex_count += 4;
        _vtxt_index_count += 6;
    }
    else
    {
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 0] = left;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 1] = bot;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 2] = glyph.min_u;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 3] = glyph.min_v;

        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 4] = right;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 5] = top;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 6] = glyph.max_u;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 7] = glyph.max_v;

        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 8] = left;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 9] = top;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 10] = glyph.min_u;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 11] = glyph.max_v;

        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 12] = right;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 13] = bot;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 14] = glyph.max_u;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 15] = glyph.min_v;

        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 16] = right;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 17] = top;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 18] = glyph.max_u;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 19] = glyph.max_v;

        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 20] = left;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 21] = bot;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 22] = glyph.min_u;
        _vtxt_vertex_buffer[_vtxt_vertex_count * STRIDE + 23] = glyph.min_v;

        _vtxt_vertex_count += 6;
    }

    // Advance the cursor
    _vtxt_cursor_x += (int) glyph.advance;
}

VTXT_DEF void
vtxt_append_glyph(const char in_glyph, vtxt_font* font, int text_height_px)
{
    __private_vtxt_append_glyph(in_glyph, font, text_height_px, 0.f);
}

VTXT_DEF void
vtxt_append_line(const char* line_of_text, vtxt_font* font, int text_height_px)
{
    int line_start_x = _vtxt_cursor_x;
    while(*line_of_text != '\0')
    {
        if(*line_of_text != '\n')
        {
            if(VTXT_MAX_CHAR_IN_BUFFER * 6 < _vtxt_vertex_count + 6) // Make sure we are not exceeding the array size
            {
                break;
            }
            vtxt_append_glyph(*line_of_text, font, text_height_px);
        }
        else
        {
            vtxt_new_line(line_start_x, font, text_height_px);
        }
        ++line_of_text;// next character
    }
}

VTXT_DEF void
vtxt_append_line_align_right(const char* line_of_text, vtxt_font* font, int text_height_px)
{
    int line_start_x = _vtxt_cursor_x;
    char line_buffer[256];
    int lb_index = 0;
    while (*line_of_text != '\0' && *line_of_text != '\n')
    {
        line_buffer[lb_index++] = *line_of_text++;
    }
    float line_length = 0.f;
    for (int i = 0; i < lb_index; ++i)
    {
        char in_glyph = line_buffer[i];
        float scale = (float)text_height_px / (float)font->font_height_px;
        vtxt_glyph glyph = font->glyphs[in_glyph - VTXT_ASCII_FROM];
        glyph.advance *= scale;
        line_length += glyph.advance;
    }
    for (int i = 0; i < lb_index; ++i)
    {
        char in_glyph = line_buffer[i];
        if (VTXT_MAX_CHAR_IN_BUFFER * 6 < _vtxt_vertex_count + 6) // Make sure we are not exceeding the array size
        {
            break;
        }
        __private_vtxt_append_glyph(in_glyph, font, text_height_px, -line_length);
    }

    if (*line_of_text == '\n')
    {
        vtxt_new_line(line_start_x, font, text_height_px);
        ++line_of_text;
        vtxt_append_line_align_right(line_of_text, font, text_height_px);
    }
    else // terminate
    {

    }
}

VTXT_DEF void
vtxt_append_line_centered(const char* line_of_text, vtxt_font* font, int text_height_px)
{
    int line_start_x = _vtxt_cursor_x;
    char line_buffer[256];
    int lb_index = 0;
    while(*line_of_text != '\0' && *line_of_text != '\n')
    {
        line_buffer[lb_index++] = *line_of_text++;
    }
    float line_length = 0.f;
    for(int i = 0; i < lb_index; ++i)
    {
        char in_glyph = line_buffer[i];
        float scale = (float)text_height_px / (float)font->font_height_px;
        vtxt_glyph glyph = font->glyphs[in_glyph - VTXT_ASCII_FROM];
        glyph.advance *= scale;
        line_length += glyph.advance;
    }
    float half_line_length = line_length/2.f;
    for(int i = 0; i < lb_index; ++i)
    {    
        char in_glyph = line_buffer[i];
        if(VTXT_MAX_CHAR_IN_BUFFER * 6 < _vtxt_vertex_count + 6) // Make sure we are not exceeding the array size
        {
            break;
        }
        __private_vtxt_append_glyph(in_glyph, font, text_height_px, -half_line_length);
    }

    if(*line_of_text == '\n')
    {
        vtxt_new_line(line_start_x, font, text_height_px);
        ++line_of_text;
        vtxt_append_line_centered(line_of_text, font, text_height_px);
    }
    else // terminate
    {

    }
}

VTXT_DEF void
vtxt_get_text_bounding_box_info(float* width_out,
                                float* height_out,
                                const char* text,
                                vtxt_font* font,
                                int text_height_px)
{
    float wSumLargestSoFar = 0;
    float wSumCurrent = 0;
    float hSum = 0;

    while (*text != '\0')
    {
        if (*text != '\n')
        {
            if (*text < VTXT_ASCII_FROM || *text > VTXT_ASCII_TO) // Make sure we have the data for this glyph
            {
                continue;
            }

            bool isLastGlyphInLine = *(text + 1) == '\0' || *(text + 1) == '\n';
            float scale = (float)text_height_px / (float)font->font_height_px;
            vtxt_glyph glyph = font->glyphs[*text - VTXT_ASCII_FROM];
            glyph.advance *= scale;
            glyph.width *= scale;
            glyph.height *= scale;
            glyph.offset_x *= scale;
            glyph.offset_y *= scale;

            wSumCurrent += isLastGlyphInLine ? glyph.offset_x + glyph.width : glyph.advance;

            if (isLastGlyphInLine)
            {
                if (wSumCurrent > wSumLargestSoFar)
                {
                    wSumLargestSoFar = wSumCurrent;
                }

                float linegap = font->linegap + _vtxt_linegap_offset;
                float heightOfThisLine = (font->ascender - font->descender + linegap) * scale;
                // Note(Kevin): honestly could prob just use the tallest glyph.height + glyph.offset_y instead
                // TODO(Kevin): Only add font->linegap if there is a new line with legit characters
                hSum += heightOfThisLine;
            }
        }
        ++text; // next character
    }

    *width_out = wSumLargestSoFar;
    *height_out = hSum;
}

VTXT_DEF vtxt_vertex_buffer
vtxt_grab_buffer()
{
    vtxt_vertex_buffer retval;
    retval.vertex_buffer = _vtxt_vertex_buffer;
    if(_vtxt_config & VTXT_CREATE_INDEX_BUFFER)
    {
        retval.vertices_array_count = _vtxt_vertex_count * 4;
        retval.index_buffer = _vtxt_index_buffer;
        retval.indices_array_count = _vtxt_index_count;
    }
    else
    {
        retval.vertices_array_count = _vtxt_vertex_count * 6;
        retval.index_buffer = NULL;
        retval.indices_array_count = 0;
    }
    retval.vertex_count = _vtxt_vertex_count;
    return retval;
}

VTXT_DEF void
vtxt_clear_buffer()
{
    // No need to actually clear the vertex and index buffers back to 0
    // Setting the counts back to 0 will suffice
    _vtxt_vertex_count = 0;
    _vtxt_index_count = 0;
}

// clean up
#undef VTXT_DEF
#undef _vtxt_internal
#undef VTXT_ASCII_FROM
#undef VTXT_ASCII_TO
#undef VTXT_MAX_CHAR_IN_BUFFER
#undef VTXT_GLYPH_COUNT
#undef VTXT_MAX_FONT_RESOLUTION
#undef VTXT_DESIRED_ATLAS_WIDTH
#undef VTXT_ATLAS_PAD_X
#undef VTXT_ATLAS_PAD_Y

#undef VERTEXT_IMPLEMENTATION
#endif // VERTEXT_IMPLEMENTATION
