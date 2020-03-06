#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_MainWin.h"

class QCustomPlot;
class Data;

class MainWin : public QMainWindow
{
	Q_OBJECT

public:
	MainWin(QWidget *parent = Q_NULLPTR);

private:
	Ui::MainWinClass ui;
	Data * data;
	QCustomPlot * plot;
	int lastId = -1;

	void setupPlot();

private slots:
	void startStop(bool toggled);
	void saveToFile();
	void clear();
	void receivedNewData();
};
