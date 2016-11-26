#include <fstream>
#include <functional>
#include <stdio.h>
#include <osal_files.h>

#include "OpenGL.h"
#include "Combiner.h"
#include "GLSLCombiner.h"
#include "UniformCollection.h"
#include "Debug.h"
#include "gDP.h"
#include "Config.h"
#include "PluginAPI.h"
#include "RSP.h"

static int saRGBExpanded[] =
{
	COMBINED,			TEXEL0,				TEXEL1,				PRIMITIVE,
	SHADE,				ENVIRONMENT,		ONE,				NOISE,
	ZERO,				ZERO,				ZERO,				ZERO,
	ZERO,				ZERO,				ZERO,				ZERO
};

static int sbRGBExpanded[] =
{
	COMBINED,			TEXEL0,				TEXEL1,				PRIMITIVE,
	SHADE,				ENVIRONMENT,		CENTER,				K4,
	ZERO,				ZERO,				ZERO,				ZERO,
	ZERO,				ZERO,				ZERO,				ZERO
};

static int mRGBExpanded[] =
{
	COMBINED,			TEXEL0,				TEXEL1,				PRIMITIVE,
	SHADE,				ENVIRONMENT,		SCALE,				COMBINED_ALPHA,
	TEXEL0_ALPHA,		TEXEL1_ALPHA,		PRIMITIVE_ALPHA,	SHADE_ALPHA,
	ENV_ALPHA,			LOD_FRACTION,		PRIM_LOD_FRAC,		K5,
	ZERO,				ZERO,				ZERO,				ZERO,
	ZERO,				ZERO,				ZERO,				ZERO,
	ZERO,				ZERO,				ZERO,				ZERO,
	ZERO,				ZERO,				ZERO,				ZERO
};

static int aRGBExpanded[] =
{
	COMBINED,			TEXEL0,				TEXEL1,				PRIMITIVE,
	SHADE,				ENVIRONMENT,		ONE,				ZERO
};

static int saAExpanded[] =
{
	COMBINED,			TEXEL0_ALPHA,		TEXEL1_ALPHA,		PRIMITIVE_ALPHA,
	SHADE_ALPHA,		ENV_ALPHA,			ONE,				ZERO
};

static int sbAExpanded[] =
{
	COMBINED,			TEXEL0_ALPHA,		TEXEL1_ALPHA,		PRIMITIVE_ALPHA,
	SHADE_ALPHA,		ENV_ALPHA,			ONE,				ZERO
};

static int mAExpanded[] =
{
	LOD_FRACTION,		TEXEL0_ALPHA,		TEXEL1_ALPHA,		PRIMITIVE_ALPHA,
	SHADE_ALPHA,		ENV_ALPHA,			PRIM_LOD_FRAC,		ZERO,
};

static int aAExpanded[] =
{
	COMBINED,			TEXEL0_ALPHA,		TEXEL1_ALPHA,		PRIMITIVE_ALPHA,
	SHADE_ALPHA,		ENV_ALPHA,			ONE,				ZERO
};

void Combiner_Init() {
	CombinerInfo & cmbInfo = CombinerInfo::get();
	cmbInfo.init();
	InitShaderCombiner();
	if (cmbInfo.getCombinersNumber() == 0) {
		gDP.otherMode.cycleType = G_CYC_COPY;
		cmbInfo.setCombine(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0));
		gDP.otherMode.cycleType = G_CYC_FILL;
		cmbInfo.setCombine(EncodeCombineMode(0, 0, 0, SHADE, 0, 0, 0, SHADE, 0, 0, 0, SHADE, 0, 0, 0, SHADE));
	}
	gDP.otherMode.cycleType = G_CYC_1CYCLE;
}

void Combiner_Destroy() {
	DestroyShaderCombiner();
	CombinerInfo::get().destroy();
}

CombinerInfo & CombinerInfo::get()
{
	static CombinerInfo info;
	return info;
}

void CombinerInfo::init()
{
	m_pCurrent = nullptr;
	m_pUniformCollection = createUniformCollection();
	GLint numBinaryFormats = 0;
#ifdef GL_NUM_PROGRAM_BINARY_FORMATS
	glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numBinaryFormats);
#endif
	m_bShaderCacheSupported = config.generalEmulation.enableShadersStorage != 0 &&
								OGLVideo::isExtensionSupported(GET_PROGRAM_BINARY_EXTENSION) &&
								numBinaryFormats > 0;

	m_shadersLoaded = 0;
	if (m_bShaderCacheSupported && !_loadShadersStorage()) {
		for (Combiners::iterator cur = m_combiners.begin(); cur != m_combiners.end(); ++cur)
			delete cur->second;
		m_combiners.clear();
	}
}

void CombinerInfo::destroy()
{
	delete m_pUniformCollection;
	m_pUniformCollection = nullptr;
	m_pCurrent = nullptr;
	if (m_bShaderCacheSupported)
		_saveShadersStorage();
	m_shadersLoaded = 0;
	for (Combiners::iterator cur = m_combiners.begin(); cur != m_combiners.end(); ++cur)
		delete cur->second;
	m_combiners.clear();
}

static
void SimplifyCycle( CombineCycle *cc, CombinerStage *stage )
{
	// Load the first operand
	stage->op[0].op = LOAD;
	stage->op[0].param1 = cc->sa;
	stage->numOps = 1;

	// If we're just subtracting zero, skip it
	if (cc->sb != ZERO) {
		// Subtracting a number from itself is zero
		if (cc->sb == stage->op[0].param1)
			stage->op[0].param1 = ZERO;
		else {
			stage->op[1].op = SUB;
			stage->op[1].param1 = cc->sb;
			stage->numOps++;
		}
	}

	// If we either subtracted, or didn't load a zero
	if ((stage->numOps > 1) || (stage->op[0].param1 != ZERO)) {
		// Multiplying by zero is zero
		if (cc->m == ZERO) {
			stage->numOps = 1;
			stage->op[0].op = LOAD;
			stage->op[0].param1 = ZERO;
		} else {
			// Multiplying by one, so just do a load
			if ((stage->numOps == 1) && (stage->op[0].param1 == ONE))
				stage->op[0].param1 = cc->m;
			else {
				stage->op[stage->numOps].op = MUL;
				stage->op[stage->numOps].param1 = cc->m;
				stage->numOps++;
			}
		}
	}

	// Don't bother adding zero
	if (cc->a != ZERO) {
		// If all we have so far is zero, then load this instead
		if ((stage->numOps == 1) && (stage->op[0].param1 == ZERO))
			stage->op[0].param1 = cc->a;
		else {
			stage->op[stage->numOps].op = ADD;
			stage->op[stage->numOps].param1 = cc->a;
			stage->numOps++;
		}
	}

	// Handle interpolation
	if ((stage->numOps == 4) && (stage->op[1].param1 == stage->op[3].param1)) {
		stage->numOps = 1;
		stage->op[0].op = INTER;
		stage->op[0].param2 = stage->op[1].param1;
		stage->op[0].param3 = stage->op[2].param1;
	}
}

ShaderCombiner * CombinerInfo::_compile(u64 mux) const
{
	gDPCombine combine;

	combine.mux = mux;

	int numCycles;

	Combiner color, alpha;

	numCycles = gDP.otherMode.cycleType + 1;
	color.numStages = numCycles;
	alpha.numStages = numCycles;

	CombineCycle cc[2];
	CombineCycle ac[2];

	// Decode and expand the combine mode into a more general form
	cc[1].sa = saRGBExpanded[combine.saRGB1];
	cc[1].sb = sbRGBExpanded[combine.sbRGB1];
	cc[1].m  = mRGBExpanded[combine.mRGB1];
	cc[1].a  = aRGBExpanded[combine.aRGB1];
	ac[1].sa = saAExpanded[combine.saA1];
	ac[1].sb = sbAExpanded[combine.sbA1];
	ac[1].m  = mAExpanded[combine.mA1];
	ac[1].a  = aAExpanded[combine.aA1];

	// Simplify each RDP combiner cycle into a combiner stage
	if (gDP.otherMode.cycleType != G_CYC_2CYCLE) {
		SimplifyCycle(&cc[1], &color.stage[0]);
		SimplifyCycle(&ac[1], &alpha.stage[0]);
	} else {
		cc[0].sa = saRGBExpanded[combine.saRGB0];
		cc[0].sb = sbRGBExpanded[combine.sbRGB0];
		cc[0].m = mRGBExpanded[combine.mRGB0];
		cc[0].a = aRGBExpanded[combine.aRGB0];
		ac[0].sa = saAExpanded[combine.saA0];
		ac[0].sb = sbAExpanded[combine.sbA0];
		ac[0].m = mAExpanded[combine.mA0];
		ac[0].a = aAExpanded[combine.aA0];

		SimplifyCycle(&cc[0], &color.stage[0]);
		SimplifyCycle(&ac[0], &alpha.stage[0]);

		const bool equalStages = (memcmp(cc, cc + 1, sizeof(CombineCycle)) | memcmp(ac, ac + 1, sizeof(CombineCycle))) == 0;
		if (!equalStages) {
			SimplifyCycle(&cc[1], &color.stage[1]);
			SimplifyCycle(&ac[1], &alpha.stage[1]);
		} else {
			color.numStages = 1;
			alpha.numStages = 1;
		}
	}

	return new ShaderCombiner( color, alpha, combine );
}

void CombinerInfo::update()
{
	// TODO: find, why gDP.changed & CHANGED_COMBINE not always works (e.g. Mario Tennis).
//	if (gDP.changed & CHANGED_COMBINE) {
		if (gDP.otherMode.cycleType == G_CYC_COPY)
			setCombine(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0));
		else if (gDP.otherMode.cycleType == G_CYC_FILL)
			setCombine(EncodeCombineMode(0, 0, 0, SHADE, 0, 0, 0, SHADE, 0, 0, 0, SHADE, 0, 0, 0, SHADE));
		else
			setCombine(gDP.combine.mux);
		gDP.changed &= ~CHANGED_COMBINE;
//	}
}

void CombinerInfo::setCombine(u64 _mux )
{
	const u64 key = getCombinerKey(_mux);
	if (m_pCurrent != nullptr && m_pCurrent->getKey() == key) {
		m_bChanged = false;
		m_pCurrent->update(false);
		return;
	}
	Combiners::const_iterator iter = m_combiners.find(key);
	if (iter != m_combiners.end()) {
		m_pCurrent = iter->second;
		m_pCurrent->update(false);
	} else {
		m_pCurrent = _compile(_mux);
		m_pCurrent->update(true);
		m_pUniformCollection->bindWithShaderCombiner(m_pCurrent);
		m_combiners[m_pCurrent->getKey()] = m_pCurrent;
	}
	m_bChanged = true;
}

void CombinerInfo::updatePrimColor()
{
	if (m_pUniformCollection != nullptr)
		m_pUniformCollection->setColorData(UniformCollection::cuPrimColor, sizeof(f32)* 5, &gDP.primColor.r);
}

void CombinerInfo::updateEnvColor()
{
	if (m_pUniformCollection != nullptr)
		m_pUniformCollection->setColorData(UniformCollection::cuEnvColor, sizeof(f32)* 4, &gDP.envColor.r);
}

void CombinerInfo::updateFogColor()
{
	if (m_pUniformCollection != nullptr)
		m_pUniformCollection->setColorData(UniformCollection::cuFogColor, sizeof(f32)* 4, &gDP.fogColor.r);
}

void CombinerInfo::updateBlendColor()
{
	if (m_pUniformCollection != nullptr)
		m_pUniformCollection->setColorData(UniformCollection::cuBlendColor, sizeof(f32)* 4, &gDP.blendColor.r);
}

void CombinerInfo::updateKeyColor()
{
	if (m_pUniformCollection != nullptr)
		m_pUniformCollection->setColorData(UniformCollection::cuCenterColor, sizeof(f32)* 8, &gDP.key.center.r);
}

void CombinerInfo::updateConvertColor()
{
	if (m_pUniformCollection == nullptr)
		return;
	f32 convert[2] = { gDP.convert.k4*0.0039215689f, gDP.convert.k5*0.0039215689f };
	m_pUniformCollection->setColorData(UniformCollection::cuK4, sizeof(convert), convert);
}

void CombinerInfo::updateTextureParameters()
{
	if (m_pUniformCollection != nullptr)
		m_pUniformCollection->updateTextureParameters();
}

void CombinerInfo::updateLightParameters()
{
	if (config.generalEmulation.enableHWLighting != 0) {
		if (m_pUniformCollection != nullptr)
			m_pUniformCollection->updateLightParameters();
	}
	gSP.changed ^= CHANGED_HW_LIGHT;
}

void CombinerInfo::updateParameters(OGLRender::RENDER_STATE _renderState)
{
	if (m_pUniformCollection != nullptr)
		m_pUniformCollection->updateUniforms(m_pCurrent, _renderState);
}

#ifndef GLES2
#define SHADER_STORAGE_FOLDER_NAME L"shaders"
static
void getStorageFileName(wchar_t * _fileName)
{
	wchar_t strCacheFolderPath[PLUGIN_PATH_SIZE];
	api().GetUserCachePath(strCacheFolderPath);
	wchar_t strShaderFolderPath[PLUGIN_PATH_SIZE];
	swprintf(strShaderFolderPath, PLUGIN_PATH_SIZE, L"%ls/%ls", strCacheFolderPath, SHADER_STORAGE_FOLDER_NAME);
	wchar_t * pPath = strShaderFolderPath;
	if (!osal_path_existsW(strShaderFolderPath) || !osal_is_directory(strShaderFolderPath)) {
		if (osal_mkdirp(strShaderFolderPath) != 0)
			pPath = strCacheFolderPath;
	}

#ifdef GLES3
	const wchar_t* strOpenGLType = L"GLES3";
#elif GLES3_1
	const wchar_t* strOpenGLType = L"GLES3_1";
#else
	const wchar_t* strOpenGLType = L"OpenGL";
#endif

	swprintf(_fileName, PLUGIN_PATH_SIZE, L"%ls/GLideN64.%08lx.%ls.shaders", pPath, std::hash<std::string>()(RSP.romname), strOpenGLType);
}

u32 CombinerInfo::_getConfigOptionsBitSet() const
{
	std::vector<u32> vecOptions;
	ShaderCombiner::getShaderCombinerOptionsSet(vecOptions);
	u32 optionsSet = 0;
	for (u32 i = 0; i < vecOptions.size(); ++i)
		optionsSet |= vecOptions[i] << i;
	return optionsSet;
}

/*
Storage format:
  uint32 - format version;
  uint32 - bitset of config options, which may change how shader is created.
  uint32 - len of renderer string
  char * - renderer string
  uint32 - len of GL version string
  char * - GL version string
  uint32 - number of shaders
  shaders in binary form
*/
static const u32 ShaderStorageFormatVersion = 0x0DU;
void CombinerInfo::_saveShadersStorage() const
{
	if (m_shadersLoaded >= m_combiners.size())
		return;

	wchar_t fileName[PLUGIN_PATH_SIZE];
	getStorageFileName(fileName);

#if defined(OS_WINDOWS) && !defined(MINGW)
	std::ofstream fout(fileName, std::ofstream::binary | std::ofstream::trunc);
#else
	char fileName_c[PATH_MAX];
	wcstombs(fileName_c, fileName, PATH_MAX);
	std::ofstream fout(fileName_c, std::ofstream::binary | std::ofstream::trunc);
#endif
	if (!fout)
		return;

	fout.write((char*)&ShaderStorageFormatVersion, sizeof(ShaderStorageFormatVersion));

	fout.write((char*)&m_configOptionsBitSet, sizeof(m_configOptionsBitSet));

	const char * strRenderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
	u32 len = strlen(strRenderer);
	fout.write((char*)&len, sizeof(len));
	fout.write(strRenderer, len);

	const char * strGLVersion = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	len = strlen(strGLVersion);
	fout.write((char*)&len, sizeof(len));
	fout.write(strGLVersion, len);

	len = m_combiners.size();
	fout.write((char*)&len, sizeof(len));
	for (Combiners::const_iterator cur = m_combiners.begin(); cur != m_combiners.end(); ++cur)
		fout << *(cur->second);
	fout.flush();
	fout.close();
}

bool CombinerInfo::_loadShadersStorage()
{
	wchar_t fileName[PLUGIN_PATH_SIZE];
	getStorageFileName(fileName);
	m_configOptionsBitSet = _getConfigOptionsBitSet();

#if defined(OS_WINDOWS) && !defined(MINGW)
	std::ifstream fin(fileName, std::ofstream::binary);
#else
	char fileName_c[PATH_MAX];
	wcstombs(fileName_c, fileName, PATH_MAX);
	std::ifstream fin(fileName_c, std::ofstream::binary);
#endif
	if (!fin)
		return false;

	try {
		u32 version;
		fin.read((char*)&version, sizeof(version));
		if (version != ShaderStorageFormatVersion)
			return false;

		u32 optionsSet;
		fin.read((char*)&optionsSet, sizeof(optionsSet));
		if (optionsSet != m_configOptionsBitSet)
			return false;

		const char * strRenderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
		u32 len;
		fin.read((char*)&len, sizeof(len));
		std::vector<char> strBuf(len);
		fin.read(strBuf.data(), len);
		if (strncmp(strRenderer, strBuf.data(), len) != 0)
			return false;

		const char * strGLVersion = reinterpret_cast<const char *>(glGetString(GL_VERSION));
		fin.read((char*)&len, sizeof(len));
		strBuf.resize(len);
		fin.read(strBuf.data(), len);
		if (strncmp(strGLVersion, strBuf.data(), len) != 0)
			return false;

		fin.read((char*)&len, sizeof(len));
		for (u32 i = 0; i < len; ++i) {
			m_pCurrent = new ShaderCombiner();
			fin >> *m_pCurrent;
			m_pCurrent->update(true);
			m_pUniformCollection->bindWithShaderCombiner(m_pCurrent);
			m_combiners[m_pCurrent->getKey()] = m_pCurrent;
		}
	}
	catch (...) {
		m_shadersLoaded = 0;
		return false;
	}

	m_shadersLoaded = m_combiners.size();
	fin.close();
	return !isGLError();
}
#else // GLES2
void CombinerInfo::_saveShadersStorage() const
{}

bool CombinerInfo::_loadShadersStorage()
{
	return true;
}
#endif //GLES2
