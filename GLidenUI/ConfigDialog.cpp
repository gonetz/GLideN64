#include <QFontDialog>
#include "ConfigDialog.h"
#include "ui_configDialog.h"
#include "FullscreenResolutions.h"

#include "../Config.h"

const unsigned int g_uMegabyte = 1024U * 1024U;

static
const unsigned int numWindowedModes = 12U;
static
struct
{
	unsigned short width, height;
	const char *description;
} WindowedModes[numWindowedModes] = {
	{ 320, 240, "320 x 240" },
	{ 400, 300, "400 x 300" },
	{ 480, 360, "480 x 360" },
	{ 640, 480, "640 x 480" },
	{ 800, 600, "800 x 600" },
	{ 960, 720, "960 x 720" },
	{ 1024, 768, "1024 x 768" },
	{ 1152, 864, "1152 x 864" },
	{ 1280, 960, "1280 x 960" },
	{ 1280, 1024, "1280 x 1024" },
	{ 1440, 1080, "1440 x 1080" },
	{ 1600, 1200, "1600 x 1200" }
};

static const unsigned int numFilters = 7U;
static const char * cmbTexFilter_choices[numFilters] = {
	"None",
	"Smooth filtering 1",
	"Smooth filtering 2",
	"Smooth filtering 3",
	"Smooth filtering 4",
	"Sharp filtering 1",
	"Sharp filtering 2"
};

static const unsigned int numEnhancements = 13U;
static const char * cmbTexEnhancement_choices[numEnhancements] = {
	"None",
	"Store",
	"X2",
	"X2SAI",
	"HQ2X",
	"HQ2XS",
	"LQ2X",
	"LQ2XS",
	"HQ4X",
	"2xBRZ",
	"3xBRZ",
	"4xBRZ",
	"5xBRZ"
};

ConfigDialog::ConfigDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ConfigDialog)
{
	ui->setupUi(this);

	// Video settings
	QStringList windowedModesList;
	int windowedModesCurrent = 0;
	for (int i = 0; i < numWindowedModes; ++i) {
		windowedModesList.append(WindowedModes[i].description);
		if (WindowedModes[i].width == config.video.windowedWidth && WindowedModes[i].height == config.video.windowedHeight)
			windowedModesCurrent = i;
	}
	ui->windowedResolutionComboBox->insertItems(0, windowedModesList);
	ui->windowedResolutionComboBox->setCurrentIndex(windowedModesCurrent);

	QStringList fullscreenModesList, fullscreenRatesList;
	int fullscreenMode, fullscreenRate;
	fillFullscreenResolutionsList(fullscreenModesList, fullscreenMode, fullscreenRatesList, fullscreenRate);
	ui->fullScreenResolutionComboBox->insertItems(0, fullscreenModesList);
	ui->fullScreenResolutionComboBox->setCurrentIndex(fullscreenMode);
	ui->fullScreenRefreshRateComboBox->insertItems(0, fullscreenRatesList);
	ui->fullScreenRefreshRateComboBox->setCurrentIndex(fullscreenRate);

	ui->verticalSyncCheckBox->setChecked(config.video.verticalSync != 0);
	ui->aliasingSlider->setValue(config.video.multisampling);
	ui->anisotropicSlider->setValue(config.video.anisotropic);
	ui->forceBilinearCheckBox->setChecked(config.texture.forceBilinear != 0);
	ui->cacheSizeSpinBox->setValue(config.texture.maxBytes/g_uMegabyte);

	ui->screenshotFormatComboBox->setCurrentIndex(config.texture.screenShotFormat);

	// Emulation settings
	ui->emulateLodCheckBox->setChecked(config.generalEmulation.enableLOD != 0);
	ui->emulateNoiseCheckBox->setChecked(config.generalEmulation.enableNoise != 0);
	ui->emulateFogCheckBox->setChecked(config.generalEmulation.enableFog != 0);
	ui->enableHWLightingCheckBox->setChecked(config.generalEmulation.enableHWLighting != 0);

	ui->frameBufferGroupBox->setChecked(config.frameBufferEmulation.enable != 0);
	ui->copyFrameCheckBox->setChecked(config.frameBufferEmulation.copyToRDRAM != 0);
	ui->RenderFBCheckBox->setChecked(config.frameBufferEmulation.copyFromRDRAM != 0);
	ui->detectCPUWritesCheckBox->setChecked(config.frameBufferEmulation.ignoreCFB == 0);
	ui->CopyDepthCheckBox->setChecked(config.frameBufferEmulation.copyDepthToRDRAM != 0);
	ui->n64DepthCompareCheckBox->setChecked(config.frameBufferEmulation.N64DepthCompare != 0);
	switch (config.frameBufferEmulation.aspect) {
		case 0:
			ui->aspectStretchRadioButton->setChecked(true);
		break;
		case 1:
			ui->aspect43RadioButton->setChecked(true);
		break;
		case 2:
			ui->aspect169RadioButton->setChecked(true);
		break;
	}

	// Texture filter settings
	QStringList textureFiltersList;
	for (int i = 0; i < numFilters; ++i)
		textureFiltersList.append(cmbTexFilter_choices[i]);
	ui->filterComboBox->insertItems(0, textureFiltersList);
	ui->filterComboBox->setCurrentIndex(config.textureFilter.txFilterMode);

	QStringList textureEnhancementList;
	for (int i = 0; i < numEnhancements; ++i)
		textureEnhancementList.append(cmbTexEnhancement_choices[i]);
	ui->enhancementComboBox->insertItems(0, textureEnhancementList);
	ui->enhancementComboBox->setCurrentIndex(config.textureFilter.txEnhancementMode);

	ui->textureFilterCacheSpinBox->setValue(config.textureFilter.txCacheSize/g_uMegabyte);
	ui->filterForce16bppCheckBox->setChecked(config.textureFilter.txFilterForce16bpp != 0);
	ui->ignoreBackgroundsCheckBox->setChecked(config.textureFilter.txFilterIgnoreBG != 0);
	ui->compressFilterCacheCheckBox->setChecked(config.textureFilter.txFilterCacheCompression != 0);

	ui->texturePackGroupBox->setChecked(config.textureFilter.txHiresEnable != 0);
	ui->hiresForce16bppCheckBox->setChecked(config.textureFilter.txHiresForce16bpp != 0);
	ui->alphaChannelCheckBox->setChecked(config.textureFilter.txHiresFullAlphaChannel != 0);
	ui->compressHDTexturesCacheCheckBox->setChecked(config.textureFilter.txHiresCacheCompression != 0);
	ui->alternativeCRCCheckBox->setChecked(config.textureFilter.txHresAltCRC != 0);
	ui->textureDumpCheckBox->setChecked(config.textureFilter.txDump != 0);
	ui->saveTextureCacheCheckBox->setChecked(config.textureFilter.txSaveCache != 0);

	QString fontName(config.font.name.c_str());
	m_font = QFont(fontName.left(fontName.indexOf(".ttf")), config.font.size);
	QString strSize;
	strSize.setNum(m_font.pointSize());
	ui->fontNameLabel->setText(m_font.family() + " - " + strSize);

	m_color = QColor(config.font.color[0], config.font.color[1], config.font.color[2]);
	ui->fontColorLabel->setFont(m_font);
	ui->fontColorLabel->setText(m_color.name());
	QPalette palette;
	palette.setColor(QPalette::Window, Qt::black);
	palette.setColor(QPalette::WindowText, m_color);
	ui->fontColorLabel->setAutoFillBackground(true);
	ui->fontColorLabel->setPalette(palette);
}

ConfigDialog::~ConfigDialog()
{
	delete ui;
}

void ConfigDialog::accept()
{
	const int currentWindowedResolution = ui->windowedResolutionComboBox->currentIndex();
	config.video.windowedWidth = WindowedModes[currentWindowedResolution].width;
	config.video.windowedHeight = WindowedModes[currentWindowedResolution].height;

	getFullscreenResolutions(ui->fullScreenResolutionComboBox->currentIndex(), config.video.fullscreenWidth, config.video.fullscreenHeight);
	getFullscreenRefreshRate(ui->fullScreenRefreshRateComboBox->currentIndex(), config.video.fullscreenRefresh);

	config.video.verticalSync = ui->verticalSyncCheckBox->isChecked() ? 1 : 0;
	config.video.multisampling = ui->aliasingSlider->value();
	config.video.anisotropic = ui->anisotropicSlider->value();
	config.texture.forceBilinear = ui->forceBilinearCheckBox->isChecked();
	config.texture.maxBytes = ui->cacheSizeSpinBox->value() * g_uMegabyte;

	config.texture.screenShotFormat = ui->screenshotFormatComboBox->currentIndex();

	// Emulation settings
	config.generalEmulation.enableLOD = ui->emulateLodCheckBox->isChecked() ? 1 : 0;
	config.generalEmulation.enableNoise = ui->emulateNoiseCheckBox->isChecked() ? 1 : 0;
	config.generalEmulation.enableFog = ui->emulateFogCheckBox->isChecked() ? 1 : 0;
	config.generalEmulation.enableHWLighting = ui->enableHWLightingCheckBox->isChecked() ? 1 : 0;

	config.frameBufferEmulation.enable = ui->frameBufferGroupBox->isChecked() ? 1 : 0;
	config.frameBufferEmulation.copyToRDRAM = ui->copyFrameCheckBox->isChecked() ? 1 : 0;
	config.frameBufferEmulation.copyFromRDRAM = ui->RenderFBCheckBox->isChecked() ? 1 : 0;
	config.frameBufferEmulation.ignoreCFB = ui->detectCPUWritesCheckBox->isChecked() ? 0 : 1;
	config.frameBufferEmulation.copyDepthToRDRAM = ui->CopyDepthCheckBox->isChecked() ? 1 : 0;
	config.frameBufferEmulation.N64DepthCompare = ui->n64DepthCompareCheckBox->isChecked() ? 1 : 0;
	if (ui->aspectStretchRadioButton->isChecked())
		config.frameBufferEmulation.aspect = 0;
	else if (ui->aspect43RadioButton->isChecked())
		config.frameBufferEmulation.aspect = 1;
	else if (ui->aspect169RadioButton->isChecked())
		config.frameBufferEmulation.aspect = 2;

	// Texture filter settings
	config.textureFilter.txFilterMode = ui->filterComboBox->currentIndex();
	config.textureFilter.txEnhancementMode = ui->enhancementComboBox->currentIndex();

	config.textureFilter.txCacheSize = ui->textureFilterCacheSpinBox->value() * g_uMegabyte;
	config.textureFilter.txFilterForce16bpp = ui->filterForce16bppCheckBox->isChecked() ? 1 : 0;
	config.textureFilter.txFilterIgnoreBG = ui->ignoreBackgroundsCheckBox->isChecked() ? 1 : 0;
	config.textureFilter.txFilterCacheCompression = ui->compressFilterCacheCheckBox->isChecked() ? 1 : 0;

	config.textureFilter.txHiresEnable = ui->texturePackGroupBox->isChecked() ? 1 : 0;
	config.textureFilter.txHiresForce16bpp = ui->hiresForce16bppCheckBox->isChecked() ? 1 : 0;
	config.textureFilter.txHiresFullAlphaChannel = ui->alphaChannelCheckBox->isChecked() ? 1 : 0;
	config.textureFilter.txHiresCacheCompression = ui->compressHDTexturesCacheCheckBox->isChecked() ? 1 : 0;
	config.textureFilter.txHresAltCRC = ui->alternativeCRCCheckBox->isChecked() ? 1 : 0;
	config.textureFilter.txDump = ui->textureDumpCheckBox->isChecked() ? 1 : 0;
	config.textureFilter.txSaveCache = ui->saveTextureCacheCheckBox->isChecked() ? 1 : 0;

	config.font.size = m_font.pointSize();
	QString fontName = m_font.family() + ".ttf";
#ifdef OS_WINDOWS
	config.font.name = fontName.toLocal8Bit().constData();
#else
	config.font.name = fontName.toStdString();
#endif

	QDialog::accept();
}

void ConfigDialog::on_selectFontButton_clicked()
{
	bool ok;
	m_font = QFontDialog::getFont(
		&ok, m_font, this);
	if (!ok)
		return;

	// the user clicked OK and font is set to the font the user selected
	QString strSize;
	strSize.setNum(m_font.pointSize());
	ui->fontNameLabel->setText(m_font.family() + " - " + strSize);
	ui->fontColorLabel->setFont(m_font);
}
