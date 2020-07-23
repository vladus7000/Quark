#pragma once

#include <Types.hpp>
#includ "RenderCommandType.hpp"

enum class RenderCommandQueueType : uint8
{
	None = 0x00,
	Copy = 0x01,
	Compute = 0x02,
	Graphics = 0x04,
	All = Copy | Compute | Graphics
};

struct RenderCommand
{
	RenderCommandType type = RenderCommandType::Count;
};

template<RenderCommandType TYPE, RenderCommandQueueType QUEUETYPE>
struct RenderCommandTyped : RenderCommand
{
	static const RenderCommandType Type = TYPE;
	static const RenderCommandQueueType QueueType = QUEUETYPE;
	
	RenderCommandTyped() { type = Type; }
}