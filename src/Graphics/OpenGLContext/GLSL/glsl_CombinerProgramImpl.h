#pragma once
#include <Graphics/CombinerProgram.h>

namespace opengl {
namespace glsl {

	class CombinerProgramImpl : public graphics::CombinerProgram
	{
	public:
		CombinerProgramImpl();
		~CombinerProgramImpl();

		void activate() override;
		void update(bool _force) override;
		CombinerKey getKey() const override;
	};

}
}