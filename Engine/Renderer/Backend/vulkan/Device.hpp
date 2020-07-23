

class VulkanCmdBuffer
{
	public:
	
	bool compileCommandBuffer()
	{
		#define COMPILE_PACKET(TYPE_STRUCT)\
		case TYPE_STRUCT::Type:\
		if (!compileCommand(*static_cast<const TYPE_STRUCT*>(command)))\
		return false;\
		break;
		
		for (const auto* command : recordedCommands)
		{
			switch(command->type)
			{
				COMPILE_PACKET(RenderCommandDispatch);
				default:
					return false;
				
			}
		}
		
		#undef COMPILE_PACKET
	}
	
	bool compileCommand(const RenderCommandDispatch& command)
	{
		
	}
}