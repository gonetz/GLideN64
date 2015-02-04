#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>

namespace Ui {
class ConfigDialog;
}

class QAbstractButton;
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

	void on_PickFontColorButton_clicked();

	void on_buttonBox_clicked(QAbstractButton *button);

private:
	void _init();

	Ui::ConfigDialog *ui;
	QFont m_font;
	QColor m_color;
};

#endif // CONFIGDIALOG_H
