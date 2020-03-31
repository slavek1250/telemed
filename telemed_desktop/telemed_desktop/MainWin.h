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
	enum Graph {
		IR,
		RED,
		BEAT,
		HR
	};

	Ui::MainWinClass ui;
	Data * data;
	QCustomPlot * plot;
	double lastCustomPlotMsMainData = -1.0;
	qint64 lastHRMs = -1;

	const QString APP_NAME = "Heart rate analyzer";

	void closeEvent(QCloseEvent *event) override;

	void setupPlot();
	void titleUnsaved();
	void titleSaved();
	void setGraphVisible(Graph graph, bool visible);

private slots:
	void startStop(bool toggled);
	void saveToFile();
	void clear();
	void receivedNewData();
	void setRedLedGraphVisible(bool visible);
	void setIrLedGraphVisible(bool visible);
	void setBeatGraphVisible(bool visible);
	void setHRGraphVisible(bool visible);
	void updateRange();
};
