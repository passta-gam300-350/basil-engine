#version 450 core

// No output needed - we're only writing to depth buffer
// Fragment shader can be minimal for depth-only rendering

void main()
{
    // OpenGL will automatically write depth values
    // No explicit color output needed for depth-only rendering
}
