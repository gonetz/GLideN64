#ifndef _UI_ABOUTDIALOG_H
#define _UI_ABOUTDIALOG_H

#include <QDialog>

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AboutDialog(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
	~AboutDialog();

private:
	void _init();
	Ui::AboutDialog *ui;
};

#endif // _UI_ABOUTDIALOG_H
