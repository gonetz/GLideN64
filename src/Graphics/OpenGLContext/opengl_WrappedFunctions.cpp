#include "opengl_WrappedFunctions.h"

namespace opengl {

	std::unique_ptr<char[]> GlVertexAttribPointerUnbufferedCommand::m_attribsData;
	std::unordered_map<int, std::shared_ptr<std::vector<u8>>> GlMapBufferRangeReadAsyncCommand::m_data;
	std::mutex GlMapBufferRangeReadAsyncCommand::m_mapMutex;
}
