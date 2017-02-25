#include "CombinerProgram.h"
#include <Config.h>

namespace graphics {

	void CombinerProgram::getShaderCombinerOptionsSet(std::vector<u32> & _vecOptions)
	{
		// WARNING: Shader Storage format version must be increased after any change in this function.
		_vecOptions.push_back(config.video.multisampling > 0 ? 1 : 0);
		_vecOptions.push_back(config.texture.bilinearMode);
		_vecOptions.push_back(config.generalEmulation.enableHWLighting);
		_vecOptions.push_back(config.generalEmulation.enableNoise);
		_vecOptions.push_back(config.generalEmulation.enableLOD);
		_vecOptions.push_back(config.frameBufferEmulation.N64DepthCompare);
		_vecOptions.push_back(config.generalEmulation.enableLegacyBlending);
		_vecOptions.push_back(config.generalEmulation.enableFragmentDepthWrite);
	}

}
