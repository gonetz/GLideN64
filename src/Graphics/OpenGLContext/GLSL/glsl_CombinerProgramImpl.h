#pragma once
#include <Graphics/CombinerProgram.h>

namespace glsl {

	class CombinerInputs
	{
	public:
		explicit CombinerInputs(int _inputs) : m_inputs(_inputs) {}

		bool usesTile(u32 _t) const;

		bool usesTexture() const;

		bool usesLOD() const;

		bool usesShade() const;

		bool usesShadeColor() const;

		bool usesHwLighting() const;

		void addInput(int _input);

	private:
		int m_inputs;
	};

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
