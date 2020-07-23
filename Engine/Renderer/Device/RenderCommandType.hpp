#pragma once

#include <Types.hpp>

enum class RenderCommandType
{
	Draw,
	DrawIndirect,
	Dispatch,
	DispatchIndirect,
	UpdateBuffer,
	UpdateTexture,
	CopyBuffer,
	CopyTexture,
	Barrier,
	Transition,
	BeginTiming,
	EndTiming,
	ResolveTimings,
	BeginEvent,
	EndEvent,
	BeginRenderPass,
	EndRenderPass,
	RayTrace,
	UpdateTopLevel,
	UpdateBottomLevel,
	UpdateShaderTable,
	Count
};
