#include "opengl_WrappedFunctions.h"

namespace opengl {

	std::unique_ptr<char[]> GlVertexAttribPointerUnbufferedCommand::m_attribsData;
	std::unordered_map<int, std::unique_ptr<u8[]>> GlMapBufferRangeReadAsyncCommand::m_data;
	std::unordered_map<int, int> GlMapBufferRangeReadAsyncCommand::m_sizes;
	std::mutex GlMapBufferRangeReadAsyncCommand::m_mapMutex;
}
