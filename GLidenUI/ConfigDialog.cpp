#include "ConfigDialog.h"
#include "ui_configDialog.h"

#define numWindowedModes 12

struct
{
	unsigned short width, height;
	const char *description;
} WindowedModes[12] = {
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

ConfigDialog::ConfigDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ConfigDialog)
{
	ui->setupUi(this);

	QStringList dataList;
	for (int i = 0; i < numWindowedModes; ++i)
		dataList.append(WindowedModes[i].description);

	ui->windowedResolutionComboBox->insertItems(0, dataList);
}

ConfigDialog::~ConfigDialog()
{
	delete ui;
}
