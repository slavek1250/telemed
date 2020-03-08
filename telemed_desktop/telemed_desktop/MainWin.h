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
	double lastCustomPlotMs = -1.0;

	const QString APP_NAME = "Analizator pulsu";

	void closeEvent(QCloseEvent *event) override;

	void setupPlot();
	void titleUnsaved();
	void titleSaved();

private slots:
	void startStop(bool toggled);
	void saveToFile();
	void clear();
	void receivedNewData();
};
