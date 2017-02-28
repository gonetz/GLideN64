#include <fstream>
#include <assert.h>

#include <Graphics/CombinerProgram.h>
#include <Graphics/OpenGLContext/opengl_Utils.h>
#include <Types.h>
#include <Log.h>
#include <RSP.h>
#include <PluginAPI.h>
#include <osal_files.h>
#include "glsl_Utils.h"
#include "glsl_ShaderStorage.h"
#include "glsl_CombinerProgramImpl.h"
#include "glsl_CombinerProgramUniformFactory.h"

using namespace glsl;

#define SHADER_STORAGE_FOLDER_NAME L"shaders"

static
void getStorageFileName(const opengl::GLInfo & _glinfo, wchar_t * _fileName)
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

	std::wstring strOpenGLType;

	if(_glinfo.isGLESX) {
		strOpenGLType = L"GLES";
	} else {
		strOpenGLType = L"OpenGL";
	}

	swprintf(_fileName, PLUGIN_PATH_SIZE, L"%ls/GLideN64.%08lx.%ls.shaders", pPath, std::hash<std::string>()(RSP.romname), strOpenGLType.c_str());
}

static
u32 _getConfigOptionsBitSet()
{
	std::vector<u32> vecOptions;
	graphics::CombinerProgram::getShaderCombinerOptionsSet(vecOptions);
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
static const u32 ShaderStorageFormatVersion = 0x0FU;
bool ShaderStorage::saveShadersStorage(const graphics::Combiners & _combiners) const
{
	wchar_t fileName[PLUGIN_PATH_SIZE];
	getStorageFileName(m_glinfo, fileName);

#if defined(OS_WINDOWS) && !defined(MINGW)
	std::ofstream fout(fileName, std::ofstream::binary | std::ofstream::trunc);
#else
	char fileName_c[PATH_MAX];
	wcstombs(fileName_c, fileName, PATH_MAX);
	std::ofstream fout(fileName_c, std::ofstream::binary | std::ofstream::trunc);
#endif
	if (!fout)
		return false;

	fout.write((char*)&ShaderStorageFormatVersion, sizeof(ShaderStorageFormatVersion));

	const u32 configOptionsBitSet = _getConfigOptionsBitSet();
	fout.write((char*)&configOptionsBitSet, sizeof(configOptionsBitSet));

	const char * strRenderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
	u32 len = strlen(strRenderer);
	fout.write((char*)&len, sizeof(len));
	fout.write(strRenderer, len);

	const char * strGLVersion = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	len = strlen(strGLVersion);
	fout.write((char*)&len, sizeof(len));
	fout.write(strGLVersion, len);

	len = _combiners.size();

#if 0
	fout.write((char*)&len, sizeof(len));
	for (auto cur = _combiners.begin(); cur != _combiners.end(); ++cur)
		fout << *(cur->second);
#else
	u32 totalWritten = 0;
	std::vector<char> allShaderData;

	for (auto cur = _combiners.begin(); cur != _combiners.end(); ++cur)
	{
		std::vector<char> data;
		if (cur->second->getBinaryForm(data))
		{
			allShaderData.insert(allShaderData.end(), data.begin(), data.end());
			++totalWritten;
		}
		else
		{
			LOG(LOG_ERROR, "Error while writing shader with key key=0x%016lX",
				static_cast<long unsigned int>(cur->second->getKey().getMux()));
		}
	}

	fout.write((char*)&totalWritten, sizeof(totalWritten));
	fout.write(allShaderData.data(), allShaderData.size());
#endif

	fout.flush();
	fout.close();
	return true;
}

static
CombinerProgramImpl * _readCominerProgramFromStream(std::istream & _is,
	CombinerProgramUniformFactory & _uniformFactory,
	opengl::CachedUseProgram * _useProgram)
{
	CombinerKey cmbKey;
	cmbKey.read(_is);

	int inputs;
	_is.read((char*)&inputs, sizeof(inputs));
	CombinerInputs cmbInputs(inputs);

	GLenum binaryFormat;
	GLint  binaryLength;
	_is.read((char*)&binaryFormat, sizeof(binaryFormat));
	_is.read((char*)&binaryLength, sizeof(binaryLength));
	std::vector<char> binary(binaryLength);
	_is.read(binary.data(), binaryLength);

	GLuint program = glCreateProgram();
	const bool isRect = cmbKey.isRectKey();
	glsl::Utils::locateAttributes(program, isRect, cmbInputs.usesTexture());
	glProgramBinary(program, binaryFormat, binary.data(), binaryLength);
	assert(glsl::Utils::checkProgramLinkStatus(program));

	UniformGroups uniforms;
	_uniformFactory.buildUniforms(program, cmbInputs, cmbKey, uniforms);

	return new CombinerProgramImpl(cmbKey, program, _useProgram, cmbInputs, std::move(uniforms));
}

bool ShaderStorage::loadShadersStorage(graphics::Combiners & _combiners)
{
	wchar_t fileName[PLUGIN_PATH_SIZE];
	getStorageFileName(m_glinfo, fileName);
	const u32 configOptionsBitSet = _getConfigOptionsBitSet();

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
		if (optionsSet != configOptionsBitSet)
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

		CombinerProgramUniformFactory uniformFactory(m_glinfo);

		fin.read((char*)&len, sizeof(len));
		for (u32 i = 0; i < len; ++i) {
			CombinerProgramImpl * pCombiner = _readCominerProgramFromStream(fin, uniformFactory, m_useProgram);
			pCombiner->update(true);
			_combiners[pCombiner->getKey()] = pCombiner;
		}
	}
	catch (...) {
		LOG(LOG_ERROR, "Stream error while loading shader cache! Buffer is probably not big enough");
	}

//	m_shadersLoaded = m_combiners.size();
	fin.close();
	return !opengl::Utils::isGLError();
}


ShaderStorage::ShaderStorage(const opengl::GLInfo & _glinfo, opengl::CachedUseProgram * _useProgram)
: m_glinfo(_glinfo)
, m_useProgram(_useProgram)
{
}
