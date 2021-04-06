/** kc_truetypeassembler.h 

 - kevinmkchin's TrueType Assembler -

    Do this:
        #define KC_TRUETYPEASSEMBLER_IMPLEMENTATION
    before you include this file in *one* C or C++ file to create the implementation.
    // i.e. it should look like this:
    #include ...
    #include ...
    #define STB_TRUETYPE_IMPLEMENTATION
    #include "stb_truetype.h"
    #define KC_TRUETYPEASSEMBLER_IMPLEMENTATION
    #include "kc_truetypeassembler.h"

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

TODO:
    - Option for getting vertices clip-space coordinates instead of screen-space
    - Kerning
    - Top-to-bottom text (vertical text)
Maybe:
    - Support creating font atlas for a select few characters (e.g. only the characters in 'D''O''O''M')
        this will allow larger font sizes for the few select characters and also smaller font texture atlas sizes.
*/
#ifndef _INCLUDE_KC_TRUETYPEASSEMBLER_H_
#define _INCLUDE_KC_TRUETYPEASSEMBLER_H_

#define KCTTA_ASCII_FROM ' '    // starting ASCII codepoint to collect font data for
#define KCTTA_ASCII_TO '~'      // ending ASCII codepoint to collect font data for
#define KCTTA_GLYPH_COUNT KCTTA_ASCII_TO - KCTTA_ASCII_FROM + 1

#define kctta_internal static   // kctta internal function

/** Stores a pointer to the vertex buffer assembly array and the count of vertices in the 
    array (total length of array would be count of vertices * 4).
*/
typedef struct
{
    int             vertex_count;           // count of vertices in vertex buffer array (4 elements per vertex, so vertices_array_count / 4 = vertex_count)
    int             vertices_array_count;   // count of elements in vertex buffer array
    int             indices_array_count;    // count of elements in index buffer array
    float*          vertex_buffer;          // pointer to vertex buffer array
    unsigned int*   index_buffer;           // pointer to index buffer array
} TTAVertexBuffer;

/** TTABitmap is a handle to hold a pointer to an unsigned byte bitmap in memory. Length/count
    of bitmap elements = width * height.
*/
typedef struct 
{
    int             width;      // bitmap width
    int             height;     // bitmap height
    unsigned char*  pixels;     // unsigned byte bitmap
} TTABitmap;

/** Stores information about a glyph. The glyph is identified via codepoint (ASCII).
    Check out https://learnopengl.com/img/in-practice/glyph.png 
    Learn more about glyph metrics if you want.
*/
typedef struct 
{
    int             width,height;
    float           advance,offset_x,offset_y,min_u,min_v,max_u,max_v;
    char            codepoint;
} TTAGlyph;

/** TTAFont is a handle to hold font information. It's around ~4KB, so don't copy it around all the time.
    
*/
typedef struct 
{
    float           ascender;                   // https://en.wikipedia.org/wiki/Ascender_(typography)
    float           descender;                  // https://en.wikipedia.org/wiki/Descender
    float           linegap;                    // gap between the bottom of the descender of one line to the top of the ascender of the line below
    TTABitmap       font_atlas;                 // stores the bitmap for the font texture atlas (https://en.wikipedia.org/wiki/Texture_atlas#/media/File:Texture_Atlas.png)
    TTAGlyph        glyphs[KCTTA_GLYPH_COUNT];  // array for glyphs information
} TTAFont;


/** Initializes a TTAFont font handle to store glyphs information, font information, and the font texture atlas.
    Collects the glyphs and font information from the font file (given as a binary buffer in memory)
    and generates the font texture atlas (https://en.wikipedia.org/wiki/Texture_atlas#/media/File:Texture_Atlas.png)
    Expensive, so you should only do this ONCE per font (and font size) and just keep the TTAFont around somewhere.
*/
kctta_internal void kctta_init_font(TTAFont*         font_handle,
                                    unsigned char*   font_buffer,
                                    int              font_height_in_pixels);

/** Move cursor location (cursor represents the position on the screen where text is placed)
*/
kctta_internal void kctta_move_cursor(int x, 
                                      int y);

/** Go to new line and set X location of cursor
*/
kctta_internal void kctta_new_line(int          x, 
                                   TTAFont*     font,
                                   unsigned int b_newline_below = 1);

/** Assemble quads for a line of text and append to vertex buffer.
    line_of_text is the text you want to draw e.g. "some text I want to draw".
    font is the TTAFont font handle that contains the font you want to use.
    font_size is the font height in pixels.
    b_reset_cursor_after_append is whether or not to return the cursor back
    to where it started after appending the line.
*/
kctta_internal void kctta_append_line(const char*   line_of_text,
                                      TTAFont*      font,
                                      int           font_size,
                                      unsigned int  b_reset_cursor_after_append = 0);

/** Assemble quad for a glyph and append to vertex buffer.
    font is the TTAFont font handle that contains the font you want to use.
    font_size is the font height in pixels
*/
kctta_internal void kctta_append_glyph(const char   in_glyph, 
                                       TTAFont*     font,
                                       int          font_size);

/** Get TTAVertexBuffer with a pointer to the vertex buffer array
    and vertex buffer information.
*/
kctta_internal TTAVertexBuffer kctta_grab_buffer();

/** Call before starting to append new text.
    Clears the vertex buffer that text is being appended to. 
    If you called kctta_grab_buffer and want to use the buffer you received,
    make sure you pass the buffer to OpenGL (glBufferData) or make a copy of
    the buffer before calling kctta_clear_buffer.
*/
kctta_internal void kctta_clear_buffer();

/** Sets the TrueTypeAssembler to use indexed vertices and return a index buffer
    as well when grabbing buffers. Clears vertex buffer if you switch the flag while
    there are already vertices in the vertex buffer. 
*/
kctta_internal void kctta_use_index_buffer(unsigned int b_use);

#undef kctta_internal

#endif // _INCLUDE_KC_TRUETYPEASSEMBLER_H_

#ifdef KC_TRUETYPEASSEMBLER_IMPLEMENTATION
///////////////////// IMPLEMENTATION //////////////////////////
#define kctta_internal static           // kctta internal function
#define kctta_local_persist static      // kctta local static variable
#define KCTTA_MAX_CHAR_IN_BUFFER 800    // maximum characters allowed in vertex buffer ("canvas")
#define KCTTA_MAX_FONT_RESOLUTION 100   // maximum font resolution when initializing font
#define KCTTA_DESIRED_ATLAS_WIDTH 400   // width of the font atlas
#define KCTTA_AT_PAD_X 1                // x padding between the glyph textures on the texture atlas
#define KCTTA_AT_PAD_Y 1                // y padding between the glyph textures on the texture atlas

kctta_internal int
kctta_ceil(float num) 
{
    int inum = (int)num;
    if (num == (float)inum) 
    {
        return inum;
    }
    return inum + 1;
}

// Buffers for vertices and texture_coords before they are written to GPU memory.
// If you have a pointer to these buffers, DO NOT let these buffers be overwritten
// before you bind the data to GPU memory.
kctta_local_persist float kctta_vertex_buffer[KCTTA_MAX_CHAR_IN_BUFFER * 6 * 4]; // 800 characters * 6 vertices * (2 xy + 2 uv)
kctta_local_persist int kctta_vertex_count = 0; // Each vertex takes up 4 places in the assembly_buffer
kctta_local_persist unsigned int kctta_index_buffer[KCTTA_MAX_CHAR_IN_BUFFER * 6];
kctta_local_persist int kctta_index_count = 0;

kctta_local_persist unsigned int kctta_b_use_indexed_draw_flag = 0; // Flag to indicate whether to use indexed draws

kctta_local_persist int kctta_cursor_x = 0;             // top left of the screen is pixel (0, 0), bot right of the screen is pixel (screen buffer width, screen buffer height)
kctta_local_persist int kctta_cursor_y = 100;           // cursor points to the base line at which to start drawing the glyph


kctta_internal void
kctta_init_font(TTAFont* font_handle, unsigned char* font_buffer, int font_height_in_pixels)
{
    int desired_atlas_width = KCTTA_DESIRED_ATLAS_WIDTH;

    if(font_height_in_pixels > KCTTA_MAX_FONT_RESOLUTION)
    {
        return;
    }

    // Font metrics
    stbtt_fontinfo stb_font_info;
    stbtt_InitFont(&stb_font_info, font_buffer, 0);
    float stb_scale = stbtt_ScaleForPixelHeight(&stb_font_info, (float)font_height_in_pixels);
    int stb_ascender;
    int stb_descender;
    int stb_linegap;
    stbtt_GetFontVMetrics(&stb_font_info, &stb_ascender, &stb_descender, &stb_linegap);
    font_handle->ascender = (float)stb_ascender * stb_scale;
    font_handle->descender = (float)stb_descender * stb_scale;
    font_handle->linegap = (float)stb_linegap * stb_scale;

    // LOAD GLYPH BITMAP AND INFO FOR EVERY CHARACTER WE WANT IN THE FONT
    TTABitmap temp_glyph_bitmaps[KCTTA_GLYPH_COUNT];
    int tallest_glyph_height = 0;
    int aggregate_glyph_width = 0;
    // load glyph data
    for(char char_index = KCTTA_ASCII_FROM; char_index <= KCTTA_ASCII_TO; ++char_index) // ASCII
    {
        TTAGlyph glyph;
        
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
        glyph.width = stb_width;
        glyph.height = stb_height;
        glyph.offset_x = (float)stb_offset_x;
        glyph.offset_y = (float)stb_offset_y;

        // Copy stb_bitmap_temp bitmap into glyph's pixels bitmap so we can free stb_bitmap_temp
        int iter = char_index - KCTTA_ASCII_FROM;
        temp_glyph_bitmaps[iter].pixels = (unsigned char*) calloc((size_t)glyph.width * (size_t)glyph.height, 1);
        for(int row = 0; row < glyph.height; ++row)
        {
            for(int col = 0; col < glyph.width; ++col)
            {
                // Flip the bitmap image from top to bottom to bottom to top
                temp_glyph_bitmaps[iter].pixels[row * glyph.width + col] = stb_bitmap_temp[(glyph.height - row - 1) * glyph.width + col];
            }
        }
        temp_glyph_bitmaps[iter].width = glyph.width;
        temp_glyph_bitmaps[iter].height = glyph.height;
        aggregate_glyph_width += glyph.width + KCTTA_AT_PAD_X;
        if(tallest_glyph_height < glyph.height)
        {
            tallest_glyph_height = glyph.height;
        }
        stbtt_FreeBitmap(stb_bitmap_temp, 0);

        font_handle->glyphs[iter] = glyph;
    }

    int desired_atlas_height = (tallest_glyph_height + KCTTA_AT_PAD_Y)
        * kctta_ceil((float)aggregate_glyph_width / (float)desired_atlas_width);
    // Build font atlas bitmap based on these parameters
    TTABitmap atlas;
    atlas.pixels = (unsigned char*) calloc(desired_atlas_width * desired_atlas_height, 1);
    atlas.width = desired_atlas_width;
    atlas.height = desired_atlas_height;
    // COMBINE ALL GLYPH BITMAPS INTO FONT ATLAS
    int glyph_count = KCTTA_GLYPH_COUNT;
    int atlas_x = 0;
    int atlas_y = 0;
    for(int i = 0; i < glyph_count; ++i)
    {
        TTABitmap glyph_bitmap = temp_glyph_bitmaps[i];
        if (atlas_x + glyph_bitmap.width > atlas.width) // check if move atlas bitmap cursor to next line
        {
            atlas_x = 0;
            atlas_y += tallest_glyph_height + KCTTA_AT_PAD_Y;
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

        atlas_x += glyph_bitmap.width + KCTTA_AT_PAD_X; // move the atlas bitmap cursor by glyph bitmap width

        free(glyph_bitmap.pixels);
    }
    font_handle->font_atlas = atlas;
}

kctta_internal void
kctta_move_cursor(int x, int y)
{
    kctta_cursor_x = x;
    kctta_cursor_y = y;
}

kctta_internal void
kctta_new_line(int x, TTAFont* font, unsigned int b_newline_below)
{
    kctta_cursor_x = x;
    if(b_newline_below)
    {
        kctta_cursor_y += (int) (-font->descender + font->linegap + font->ascender);
    }
    else
    {
        kctta_cursor_y -= (int) (-font->descender + font->linegap + font->ascender);
    }
}

kctta_internal void
kctta_append_glyph(const char in_glyph, TTAFont* font, int font_size)
{
    if(KCTTA_MAX_CHAR_IN_BUFFER * 6 < kctta_vertex_count + 6) // Make sure we are not exceeding the array size
    {
        return;
    }

    float scale = ((float) font_size) / (font->ascender - font->descender);
    TTAGlyph glyph = font->glyphs[in_glyph - KCTTA_ASCII_FROM];
    glyph.advance *= scale;
    glyph.width = (int) ((float) glyph.width * scale);
    glyph.height = (int) ((float) glyph.height * scale);
    glyph.offset_x *= scale;
    glyph.offset_y *= scale;

    // For each of the 6 vertices, fill in the kctta_vertex_buffer in the order x y u v
    int STRIDE = 4;

    if(kctta_b_use_indexed_draw_flag)
    {
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 0] = kctta_cursor_x + glyph.offset_x;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 1] = kctta_cursor_y + glyph.offset_y + glyph.height;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 2] = glyph.min_u;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 3] = glyph.min_v;

        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 4] = kctta_cursor_x + glyph.offset_x;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 5] = kctta_cursor_y + glyph.offset_y;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 6] = glyph.min_u;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 7] = glyph.max_v;

        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 8] = kctta_cursor_x + glyph.offset_x + glyph.width;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 9] = kctta_cursor_y + glyph.offset_y;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 10] = glyph.max_u;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 11] = glyph.max_v;

        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 12] = kctta_cursor_x + glyph.offset_x + glyph.width;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 13] = kctta_cursor_y + glyph.offset_y + glyph.height;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 14] = glyph.max_u;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 15] = glyph.min_v;

        kctta_index_buffer[kctta_index_count + 0] = kctta_vertex_count + 0;
        kctta_index_buffer[kctta_index_count + 1] = kctta_vertex_count + 1;
        kctta_index_buffer[kctta_index_count + 2] = kctta_vertex_count + 2;
        kctta_index_buffer[kctta_index_count + 3] = kctta_vertex_count + 0;
        kctta_index_buffer[kctta_index_count + 4] = kctta_vertex_count + 2;
        kctta_index_buffer[kctta_index_count + 5] = kctta_vertex_count + 3;

        kctta_vertex_count += 4;
        kctta_index_count += 6;
    }
    else
    {
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 0] = kctta_cursor_x + glyph.offset_x;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 1] = kctta_cursor_y + glyph.offset_y + glyph.height;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 2] = glyph.min_u;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 3] = glyph.min_v;

        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 4] = kctta_cursor_x + glyph.offset_x;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 5] = kctta_cursor_y + glyph.offset_y;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 6] = glyph.min_u;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 7] = glyph.max_v;

        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 8] = kctta_cursor_x + glyph.offset_x + glyph.width;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 9] = kctta_cursor_y + glyph.offset_y;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 10] = glyph.max_u;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 11] = glyph.max_v;

        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 12] = kctta_cursor_x + glyph.offset_x + glyph.width;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 13] = kctta_cursor_y + glyph.offset_y + glyph.height;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 14] = glyph.max_u;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 15] = glyph.min_v;

        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 16] = kctta_cursor_x + glyph.offset_x;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 17] = kctta_cursor_y + glyph.offset_y + glyph.height;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 18] = glyph.min_u;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 19] = glyph.min_v;

        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 20] = kctta_cursor_x + glyph.offset_x + glyph.width;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 21] = kctta_cursor_y + glyph.offset_y;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 22] = glyph.max_u;
        kctta_vertex_buffer[kctta_vertex_count * STRIDE + 23] = glyph.max_v;

        kctta_vertex_count += 6;
    }

    // Advance the cursor
    kctta_cursor_x += (int) glyph.advance;
}

kctta_internal void
kctta_append_line(const char* line_of_text, TTAFont* font, int font_size, unsigned int b_reset_cursor_after_append)
{
    int line_start_x = kctta_cursor_x;
    int line_start_y = kctta_cursor_y;
    while(*line_of_text != '\0')
    {
        if(*line_of_text != '\n')
        {
            if(KCTTA_MAX_CHAR_IN_BUFFER * 6 < kctta_vertex_count + 6) // Make sure we are not exceeding the array size
            {
                break;
            }

            kctta_append_glyph(*line_of_text, font, font_size);

        }
        else
        {
            kctta_new_line(line_start_x, font);
        }

        ++line_of_text;
    }

    if(b_reset_cursor_after_append)
    {
        kctta_cursor_x = line_start_x;
        kctta_cursor_y = line_start_y;
    }
}

kctta_internal TTAVertexBuffer
kctta_grab_buffer()
{
    TTAVertexBuffer retval;
    retval.vertex_buffer = kctta_vertex_buffer;
    if(kctta_b_use_indexed_draw_flag)
    {
        retval.index_buffer = kctta_index_buffer;
        retval.vertices_array_count = kctta_vertex_count * 4;
        retval.indices_array_count = kctta_index_count;
    }
    else
    {
        retval.index_buffer = NULL;
        retval.vertices_array_count = kctta_vertex_count * 6;
        retval.indices_array_count = 0;
    }
    retval.vertex_count = kctta_vertex_count;
    return retval;
}

kctta_internal void
kctta_clear_buffer()
{
    for(int i = 0; i < kctta_vertex_count; ++i)
    {   
        int STRIDE = 4;
        kctta_vertex_buffer[i * STRIDE + 0] = 0;
        kctta_vertex_buffer[i * STRIDE + 1] = 0;
        kctta_vertex_buffer[i * STRIDE + 2] = 0;
        kctta_vertex_buffer[i * STRIDE + 3] = 0;
    }
    for(int i = 0; i < kctta_index_count; ++i)
    {   
        kctta_vertex_buffer[i] = 0;
    }
    kctta_vertex_count = 0;
    kctta_index_count = 0;
}

kctta_internal void
kctta_use_index_buffer(unsigned int b_use)
{
    if((b_use > 0) != (kctta_b_use_indexed_draw_flag > 0))
    {
        kctta_clear_buffer();
    }
    kctta_b_use_indexed_draw_flag = b_use;
}

// clean up
#undef kctta_internal
#undef kctta_local_persist
#undef KCTTA_ASCII_FROM
#undef KCTTA_ASCII_TO
#undef KCTTA_MAX_CHAR_IN_BUFFER
#undef KCTTA_GLYPH_COUNT
#undef KCTTA_MAX_FONT_RESOLUTION
#undef KCTTA_DESIRED_ATLAS_WIDTH
#undef KCTTA_AT_PAD_X
#undef KCTTA_AT_PAD_Y

#undef KC_TRUETYPEASSEMBLER_IMPLEMENTATION
#endif // KC_TRUETYPEASSEMBLER_IMPLEMENTATION
