#pragma once

// All command types now use the efficient value-based system
#include "RenderCommandBuffer.h"

// Export the new command types
using RenderCommand = VariantRenderCommand;
namespace Commands = RenderCommands;