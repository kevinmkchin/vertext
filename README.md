# vertext
Single header C library for assembling textured quads for rendering text. This library does not perform rendering directly and instead provides a vertex buffer of vertices (in either Screen Space or Clip Space) and texture coordinates. This allows the library to work seamlessly with both OpenGL and DirectX. Very lightweight: ~700 lines of code.

PURPOSE: Text rendering is a non-trivial task, and this library strives to make it easy and frictionless.

This library REQUIRES [Sean Barrett's stb_truetype.h](https://github.com/nothings/stb/blob/master/stb_truetype.h) library (which is also a single header).

![](https://github.com/kevinmkchin/vertext/blob/main/misc/text-buffer-assembly.gif?raw=true)

![](https://github.com/kevinmkchin/vertext/blob/main/misc/console.gif?raw=true)
