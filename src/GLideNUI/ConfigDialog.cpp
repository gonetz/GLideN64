#include <QDir>
#include <QFileDialog>
#include <QFontDialog>
#include <QColorDialog>
#include <QAbstractButton>
#include <QMessageBox>

#include "../Config.h"
#include "ui_configDialog.h"
#include "Settings.h"
#include "ConfigDialog.h"
#include "FullscreenResolutions.h"

static
const unsigned int numWindowedModes = 13U;
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
	{ 1600, 1024, "1600 x 1024" },
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

void ConfigDialog::_init()
{
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

	ui->aliasingSlider->setValue(config.video.multisampling);
	ui->anisotropicSlider->setValue(config.texture.maxAnisotropy);
	ui->cacheSizeSpinBox->setValue(config.texture.maxBytes / gc_uMegabyte);

	switch (config.texture.bilinearMode) {
	case BILINEAR_3POINT:
		ui->blnr3PointRadioButton->setChecked(true);
		break;
	case BILINEAR_STANDARD:
		ui->blnrStandardRadioButton->setChecked(true);
		break;
	}

	switch (config.texture.screenShotFormat) {
	case 0:
		ui->bmpRadioButton->setChecked(true);
		break;
	case 1:
		ui->jpegRadioButton->setChecked(true);
		break;
	}

	// Emulation settings
	ui->emulateLodCheckBox->setChecked(config.generalEmulation.enableLOD != 0);
	ui->emulateNoiseCheckBox->setChecked(config.generalEmulation.enableNoise != 0);
	ui->emulateFogCheckBox->setChecked(config.generalEmulation.enableFog != 0);
	ui->enableHWLightingCheckBox->setChecked(config.generalEmulation.enableHWLighting != 0);
	ui->customSettingsCheckBox->setChecked(config.generalEmulation.enableCustomSettings != 0);

	ui->frameBufferGroupBox->setChecked(config.frameBufferEmulation.enable != 0);
	ui->copyFrameCheckBox->setChecked(config.frameBufferEmulation.copyToRDRAM != 0);
	ui->RenderFBCheckBox->setChecked(config.frameBufferEmulation.copyFromRDRAM != 0);
	ui->detectCPUWritesCheckBox->setChecked(config.frameBufferEmulation.detectCFB != 0);
	ui->CopyDepthCheckBox->setChecked(config.frameBufferEmulation.copyDepthToRDRAM != 0);
	ui->n64DepthCompareCheckBox->setChecked(config.frameBufferEmulation.N64DepthCompare != 0);
	switch (config.frameBufferEmulation.aspect) {
	case Config::aStretch:
		ui->aspectStretchRadioButton->setChecked(true);
		break;
	case Config::a43:
		ui->aspect43RadioButton->setChecked(true);
		break;
	case Config::a169:
		ui->aspect169RadioButton->setChecked(true);
		break;
	case Config::aAdjust:
		ui->aspectAdjustRadioButton->setChecked(true);
		break;
	}
	switch (config.frameBufferEmulation.validityCheckMethod) {
	case Config::vcFill:
		ui->validityMethodFillRadioButton->setChecked(true);
		break;
	case Config::vcFingerprint:
		ui->validityMethodFingerprintRadioButton->setChecked(true);
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

	ui->textureFilterCacheSpinBox->setValue(config.textureFilter.txCacheSize / gc_uMegabyte);
	ui->ignoreBackgroundsCheckBox->setChecked(config.textureFilter.txFilterIgnoreBG != 0);

	ui->texturePackGroupBox->setChecked(config.textureFilter.txHiresEnable != 0);
	ui->alphaChannelCheckBox->setChecked(config.textureFilter.txHiresFullAlphaChannel != 0);
	ui->alternativeCRCCheckBox->setChecked(config.textureFilter.txHresAltCRC != 0);
	ui->textureDumpCheckBox->setChecked(config.textureFilter.txDump != 0);
	ui->force16bppCheckBox->setChecked(config.textureFilter.txForce16bpp != 0);
	ui->compressCacheCheckBox->setChecked(config.textureFilter.txCacheCompression != 0);
	ui->saveTextureCacheCheckBox->setChecked(config.textureFilter.txSaveCache != 0);

	ui->txPathLabel->setText(QString::fromWCharArray(config.textureFilter.txPath));

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

	// Post filter settings
	ui->bloomGroupBox->setChecked(config.bloomFilter.enable != 0);
	switch (config.bloomFilter.blendMode) {
	case 0:
		ui->bloomStrongRadioButton->setChecked(true);
		break;
	case 1:
		ui->bloomMildRadioButton->setChecked(true);
		break;
	case 2:
		ui->bloomLightRadioButton->setChecked(true);
		break;
	}
	ui->bloomThresholdSlider->setValue(config.bloomFilter.thresholdLevel);
	ui->blurAmountSlider->setValue(config.bloomFilter.blurAmount);
	ui->blurStrengthSlider->setValue(config.bloomFilter.blurStrength);
}

void ConfigDialog::_getTranslations(QStringList & _translationFiles) const
{
	QDir pluginFolder(m_strIniPath);
	QStringList nameFilters("gliden64_*.qm");
	_translationFiles = pluginFolder.entryList(nameFilters, QDir::Files, QDir::Name);
}


void ConfigDialog::setIniPath(const QString & _strIniPath)
{
	m_strIniPath = _strIniPath;

	QStringList translationFiles;
	_getTranslations(translationFiles);

	const QString currentTranslation = getTranslationFile();
	int listIndex = 0;
	QStringList translationLanguages("English");
	for (int i = 0; i < translationFiles.size(); ++i) {
		// get locale extracted by filename
		QString locale = translationFiles[i]; // "TranslationExample_de.qm"
		const bool bCurrent = locale == currentTranslation;
		locale.truncate(locale.lastIndexOf('.')); // "TranslationExample_de"
		locale.remove(0, locale.indexOf('_') + 1); // "de"
		QString language = QLocale::languageToString(QLocale(locale).language());
		if (bCurrent) {
			listIndex = i + 1;
		}
		translationLanguages << language;
	}

	ui->translationsComboBox->insertItems(0, translationLanguages);
	ui->translationsComboBox->setCurrentIndex(listIndex);
}

ConfigDialog::ConfigDialog(QWidget *parent) :
QDialog(parent),
ui(new Ui::ConfigDialog),
m_accepted(false)
{
	ui->setupUi(this);
	_init();
}

ConfigDialog::~ConfigDialog()
{
	delete ui;
}

void ConfigDialog::accept()
{
	m_accepted = true;
	const int currentWindowedResolution = ui->windowedResolutionComboBox->currentIndex();
	config.video.windowedWidth = WindowedModes[currentWindowedResolution].width;
	config.video.windowedHeight = WindowedModes[currentWindowedResolution].height;

	getFullscreenResolutions(ui->fullScreenResolutionComboBox->currentIndex(), config.video.fullscreenWidth, config.video.fullscreenHeight);
	getFullscreenRefreshRate(ui->fullScreenRefreshRateComboBox->currentIndex(), config.video.fullscreenRefresh);

	config.video.multisampling = ui->aliasingSlider->value();
	config.texture.maxAnisotropy = ui->anisotropicSlider->value();
	config.texture.maxBytes = ui->cacheSizeSpinBox->value() * gc_uMegabyte;

	if (ui->blnrStandardRadioButton->isChecked())
		config.texture.bilinearMode = BILINEAR_STANDARD;
	else if (ui->blnr3PointRadioButton->isChecked())
		config.texture.bilinearMode = BILINEAR_3POINT;

	if (ui->bmpRadioButton->isChecked())
		config.texture.screenShotFormat = 0;
	else if (ui->jpegRadioButton->isChecked())
		config.texture.screenShotFormat = 1;

	const int lanuageIndex = ui->translationsComboBox->currentIndex();
	if (lanuageIndex == 0) // English
		config.translationFile.clear();
	else {
		QStringList translationFiles;
		_getTranslations(translationFiles);
		config.translationFile = translationFiles[lanuageIndex-1].toLocal8Bit().constData();
	}

	// Emulation settings
	config.generalEmulation.enableLOD = ui->emulateLodCheckBox->isChecked() ? 1 : 0;
	config.generalEmulation.enableNoise = ui->emulateNoiseCheckBox->isChecked() ? 1 : 0;
	config.generalEmulation.enableFog = ui->emulateFogCheckBox->isChecked() ? 1 : 0;
	config.generalEmulation.enableHWLighting = ui->enableHWLightingCheckBox->isChecked() ? 1 : 0;
	config.generalEmulation.enableCustomSettings = ui->customSettingsCheckBox->isChecked() ? 1 : 0;

	config.frameBufferEmulation.enable = ui->frameBufferGroupBox->isChecked() ? 1 : 0;
	config.frameBufferEmulation.copyToRDRAM = ui->copyFrameCheckBox->isChecked() ? 1 : 0;
	config.frameBufferEmulation.copyFromRDRAM = ui->RenderFBCheckBox->isChecked() ? 1 : 0;
	config.frameBufferEmulation.detectCFB = ui->detectCPUWritesCheckBox->isChecked() ? 1 : 0;
	config.frameBufferEmulation.copyDepthToRDRAM = ui->CopyDepthCheckBox->isChecked() ? 1 : 0;
	config.frameBufferEmulation.N64DepthCompare = ui->n64DepthCompareCheckBox->isChecked() ? 1 : 0;
	if (ui->aspectStretchRadioButton->isChecked())
		config.frameBufferEmulation.aspect = Config::aStretch;
	else if (ui->aspect43RadioButton->isChecked())
		config.frameBufferEmulation.aspect = Config::a43;
	else if (ui->aspect169RadioButton->isChecked())
		config.frameBufferEmulation.aspect = Config::a169;
	else if (ui->aspectAdjustRadioButton->isChecked())
		config.frameBufferEmulation.aspect = Config::aAdjust;

	if (ui->validityMethodFillRadioButton->isChecked())
		config.frameBufferEmulation.validityCheckMethod = Config::vcFill;
	else if (ui->validityMethodFingerprintRadioButton->isChecked())
		config.frameBufferEmulation.validityCheckMethod = Config::vcFingerprint;

	// Texture filter settings
	config.textureFilter.txFilterMode = ui->filterComboBox->currentIndex();
	config.textureFilter.txEnhancementMode = ui->enhancementComboBox->currentIndex();

	config.textureFilter.txCacheSize = ui->textureFilterCacheSpinBox->value() * gc_uMegabyte;
	config.textureFilter.txFilterIgnoreBG = ui->ignoreBackgroundsCheckBox->isChecked() ? 1 : 0;

	config.textureFilter.txHiresEnable = ui->texturePackGroupBox->isChecked() ? 1 : 0;
	config.textureFilter.txHiresFullAlphaChannel = ui->alphaChannelCheckBox->isChecked() ? 1 : 0;
	config.textureFilter.txHresAltCRC = ui->alternativeCRCCheckBox->isChecked() ? 1 : 0;
	config.textureFilter.txDump = ui->textureDumpCheckBox->isChecked() ? 1 : 0;

	config.textureFilter.txCacheCompression = ui->compressCacheCheckBox->isChecked() ? 1 : 0;
	config.textureFilter.txForce16bpp = ui->force16bppCheckBox->isChecked() ? 1 : 0;
	config.textureFilter.txSaveCache = ui->saveTextureCacheCheckBox->isChecked() ? 1 : 0;

	QString txPath = ui->txPathLabel->text();
	if (!txPath.isEmpty())
		config.textureFilter.txPath[txPath.toWCharArray(config.textureFilter.txPath)] = '\0';

	config.font.size = m_font.pointSize();
	QString fontName = m_font.family() + ".ttf";
#ifdef OS_WINDOWS
	config.font.name = fontName.toLocal8Bit().constData();
#else
	config.font.name = fontName.toStdString();
#endif
	config.font.color[0] = m_color.red();
	config.font.color[1] = m_color.green();
	config.font.color[2] = m_color.blue();
	config.font.color[3] = m_color.alpha();
	config.font.colorf[0] = m_color.redF();
	config.font.colorf[1] = m_color.greenF();
	config.font.colorf[2] = m_color.blueF();
	config.font.colorf[3] = m_color.alphaF();

	// Post filter settings
	config.bloomFilter.enable = ui->bloomGroupBox->isChecked() ? 1 : 0;
	if (ui->bloomStrongRadioButton->isChecked())
		config.bloomFilter.blendMode = 0;
	else if (ui->bloomMildRadioButton->isChecked())
		config.bloomFilter.blendMode = 1;
	else if (ui->bloomLightRadioButton->isChecked())
		config.bloomFilter.blendMode = 2;
	config.bloomFilter.thresholdLevel = ui->bloomThresholdSlider->value();
	config.bloomFilter.blurAmount = ui->blurAmountSlider->value();
	config.bloomFilter.blurStrength = ui->blurStrengthSlider->value();

	writeSettings(m_strIniPath);

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

void ConfigDialog::on_PickFontColorButton_clicked()
{
	const QColor color = QColorDialog::getColor(m_color, this);

	if (!color.isValid())
		return;

	m_color = color;
	QPalette palette;
	palette.setColor(QPalette::Window, Qt::black);
	palette.setColor(QPalette::WindowText, m_color);
	ui->fontColorLabel->setText(m_color.name());
	ui->fontColorLabel->setPalette(palette);
}

void ConfigDialog::on_buttonBox_clicked(QAbstractButton *button)
{
	if ((QPushButton *)button == ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)) {
		QMessageBox msgBox(QMessageBox::Question, "GLideN64",
			"Do you really want to reset all settings to defaults?",
			QMessageBox::RestoreDefaults | QMessageBox::Cancel, this
			);
		msgBox.setDefaultButton(QMessageBox::Cancel);
		if (msgBox.exec() == QMessageBox::RestoreDefaults) {
			config.resetToDefaults();
			_init();
		}
	}
}

void ConfigDialog::on_fullScreenResolutionComboBox_currentIndexChanged(int index)
{
	QStringList fullscreenRatesList;
	int fullscreenRate;
	fillFullscreenRefreshRateList(index, fullscreenRatesList, fullscreenRate);
	ui->fullScreenRefreshRateComboBox->clear();
	ui->fullScreenRefreshRateComboBox->insertItems(0, fullscreenRatesList);
	ui->fullScreenRefreshRateComboBox->setCurrentIndex(fullscreenRate);
}

void ConfigDialog::on_texPackPathButton_clicked()
{
	QFileDialog::Options options = QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly | QFileDialog::ReadOnly | QFileDialog::DontUseSheet | QFileDialog::ReadOnly | QFileDialog::HideNameFilterDetails;
	QString directory = QFileDialog::getExistingDirectory(this,
		"",
		QString::fromWCharArray(config.textureFilter.txPath),
		options);
	if (!directory.isEmpty())
		ui->txPathLabel->setText(directory);
}
