#pragma once
#include <memory>
#include <vector>
#include <Graphics/CombinerProgram.h>
#include "glsl_CombinerInputs.h"

namespace glsl {

	class UniformGroup {
	public:
		virtual ~UniformGroup() {}
		virtual void update(bool _force) = 0;
	};

	typedef std::vector< std::unique_ptr<UniformGroup> > UniformGroups;

	class CombinerProgramImpl : public graphics::CombinerProgram
	{
	public:
		CombinerProgramImpl(const CombinerKey & _key,
			GLuint _program,
			const CombinerInputs & _inputs,
			UniformGroups && _uniforms);
		~CombinerProgramImpl();

		void activate() override;
		void update(bool _force) override;
		CombinerKey getKey() const override;

		bool usesTexture() const override;
		bool usesTile(u32 _t) const override;
		bool usesShade() const override;
		bool usesLOD() const override;

		bool getBinaryForm(std::vector<char> & _buffer) override;

	private:
		bool m_bNeedUpdate;
		CombinerKey m_key;
		GLuint m_program;
		CombinerInputs m_inputs;
		UniformGroups m_uniforms;
	};

}
