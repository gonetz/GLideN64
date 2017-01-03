#pragma once
#include <Combiner.h>
#include <Graphics/CombinerProgram.h>

namespace opengl {
namespace glsl {

	class CombinerProgramBuilder
	{
	public:
		CombinerProgramBuilder();
		~CombinerProgramBuilder();
		graphics::CombinerProgram * buildCombinerProgram(Combiner & _color, Combiner & _alpha, const CombinerKey & _key);
	};

}
}