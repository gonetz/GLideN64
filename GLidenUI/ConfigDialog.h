#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>

extern const unsigned int g_uMegabyte;

namespace Ui {
class ConfigDialog;
}

class ConfigDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ConfigDialog(QWidget *parent = 0);
	~ConfigDialog();

public Q_SLOTS:
	virtual void accept();

private:
	Ui::ConfigDialog *ui;
};

#endif // CONFIGDIALOG_H
