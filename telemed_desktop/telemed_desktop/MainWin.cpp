#include "MainWin.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QCustomPlot.h>
#include "Data.h"

MainWin::MainWin(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	ui.tableDockWgt->setVisible(false);

	data = new Data(this);
	plot = new QCustomPlot(this);

	ui.plotLayout->addWidget(plot);
	setupPlot();

	connect(ui.actionStartStop, &QAction::toggled, this, &MainWin::startStop);
	connect(ui.actionSaveAs, &QAction::triggered, this, &MainWin::saveToFile);
	connect(ui.actionClear, &QAction::triggered, this, &MainWin::clear);
	connect(data, &Data::receivedNewData, this, &MainWin::receivedNewData);
}

void MainWin::startStop(bool toggled) {
	ui.actionStartStop->setText(toggled ? "Start" : "Stop");
	if (toggled) {
		data->stop();
	}
	else {
		data->start();
	}
}

void MainWin::setupPlot() {
	plot->addGraph();	
	plot->graph()->setName("Test");
	plot->legend->setVisible(true);
	plot->legend->setBrush(QColor(255, 255, 255, 150));
}

void MainWin::saveToFile() {
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save as"),
		QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
		tr("Comma separated file (*.csv)"));
	data->saveAs(fileName);
}

void MainWin::clear() {
	plot->clearGraphs();
	setupPlot();
	data->clear();
	lastId = -1;
	plot->replot();
}

void MainWin::receivedNewData() {
	plot->graph()->addData(
		data->getXData(lastId + 1),
		data->getYData(lastId + 1)
	);
	lastId = data->getLastId();
	plot->rescaleAxes();
	plot->replot();
}