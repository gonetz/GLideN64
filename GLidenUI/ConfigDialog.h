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

private slots:
	void on_selectFontButton_clicked();

private:
	Ui::ConfigDialog *ui;
	QFont m_font;
	QColor m_color;
};

#endif // CONFIGDIALOG_H
