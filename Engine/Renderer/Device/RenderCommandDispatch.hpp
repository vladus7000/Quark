#pragma once

#include <Types.hpp>
#include "RenderCommandDispatch.hpp"

struct RenderCommandDispatch : RenderCommandTyped<RenderCommandType::Dispatch, RenderCommandQueueType::Compute>
{
	RenderResourceHandle pipelineState;
	//ShaderArgument shaderArguments[MaxShaderParameters];
	uint32 shaderArgumentsCount = 0;
	
	uint32 dispatchX = 0;
	uint32 dispatchY = 0;
	uint32 dispatchZ = 0;
}