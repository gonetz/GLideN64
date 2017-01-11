#ifndef GRAPHICS_DRAWERIMPL_H
#define GRAPHICS_DRAWERIMPL_H
#include <GraphicsDrawer.h>
#include "Parameter.h"
#include "CombinerProgram.h"

namespace graphics {

	class DrawerImpl {
	public:
		virtual ~DrawerImpl() {}

		struct DrawTriangleParameters
		{
			Parameter mode;
			Parameter elementsType;
			u32 verticesCount = 0;
			u32 elementsCount = 0;
			bool flatColors = false;
			SPVertex * vertices = nullptr;
			void * elements = nullptr;
			const CombinerProgram * combiner = nullptr;
		};
		virtual void drawTriangles(const DrawTriangleParameters & _params) = 0;

		struct DrawRectParameters
		{
			Parameter mode;
			u32 verticesCount = 0;
			f32 rectColor[4];
			RectVertex * vertices = nullptr;
			const CombinerProgram * combiner = nullptr;
		};
		virtual void drawRects(const DrawRectParameters & _params) = 0;

	};
}


#endif // GRAPHICS_DRAWERIMPL_H