#pragma once
#include <vector>
#include "CombinerKey.h"

namespace graphics {

	class CombinerProgram
	{
	public:
		virtual ~CombinerProgram() {}
		virtual void activate() = 0;
		virtual void update(bool _force) = 0;
		virtual CombinerKey getKey() const = 0;
		virtual bool usesTexture() const = 0;
		virtual bool usesTile(u32 _t) const = 0;
		virtual bool usesShade() const = 0;
		virtual bool usesLOD() const = 0;

		// TODO implement
		void disableBlending() {}
		void updateFrameBufferInfo(bool _bForce = false) {}

		friend std::ostream & operator<< (std::ostream & _os, const CombinerProgram & _combiner);
		friend std::istream & operator>> (std::istream & _os, CombinerProgram & _combiner);

		static void getShaderCombinerOptionsSet(std::vector<u32> & _vecOptions);
	};

}
