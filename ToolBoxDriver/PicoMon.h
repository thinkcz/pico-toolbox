#pragma once

#define TOOL_PICOMON_UID  1

extern "C" {

	unsigned int PicoMon_ToolDispatcher(unsigned int uCmd, void* pData, size_t sDataSize);


}
