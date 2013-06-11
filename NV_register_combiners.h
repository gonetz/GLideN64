#include <GL/gl.h>

struct CombinerInput
{
	GLenum input;
	GLenum mapping;
	GLenum usage;
};

struct CombinerVariable
{
	GLenum input;
	GLenum mapping;
	GLenum usage;
	BOOL used;
};

struct GeneralCombiner
{
	CombinerVariable A, B, C, D;

	struct
	{
		GLenum ab;
		GLenum cd;
		GLenum sum;
	} output;
};

class RegisterCombiners : public OGLCombiner
{
public:
	RegisterCombiners(Combiner *_color, Combiner *_alpha);
	virtual void Set();
	virtual void UpdateColors();
	virtual void UpdateFBInfo() {};

private:
	GeneralCombiner m_color[8];
	GeneralCombiner m_alpha[8];

	struct
	{
		CombinerVariable A, B, C, D, E, F, G;
	} m_final;

	struct 
	{
		WORD color, alpha;
	} m_constant[2];

	struct
	{
		WORD color, secondaryColor, alpha;
	} m_vertex;

	WORD m_numCombiners;
	BOOL m_usesT0, m_usesT1, m_usesNoise;
};

void Init_NV_register_combiners();
