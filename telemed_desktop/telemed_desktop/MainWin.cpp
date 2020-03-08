#include "MainWin.h"
#include <QCloseEvent>
#include <QFileDialog>
#include <QStandardPaths>
#include <QCustomPlot.h>
#include "Data.h"

MainWin::MainWin(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	ui.tableDockWgt->setVisible(false);
	titleSaved();

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
	//plot->xAxis
	QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
	dateTicker->setDateTimeFormat("mm:ss.zzz");
	plot->xAxis->setTicker(dateTicker);
	//plot->xAxis.setDateTimeFormat("mm:ss:zzz");
	//plot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
}

void MainWin::titleUnsaved() {
	this->setWindowTitle(APP_NAME + "*");
}

void MainWin::titleSaved() {
	this->setWindowTitle(APP_NAME);
}

void MainWin::saveToFile() {
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save as"),
		QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
		tr("Comma separated file (*.csv)"));
	data->saveAs(fileName);
	titleSaved();
}

void MainWin::clear() {
	if (!data->isDataSaved()) {
		auto ans = QMessageBox::question(this, APP_NAME,
			"Wymazaæ niezapisane dane?",
			QMessageBox::Cancel,
			QMessageBox::Yes);
		if (ans != QMessageBox::Yes) {
			return;
		}
	}
	titleSaved();
	plot->clearGraphs();
	setupPlot();
	data->clear();
	lastCustomPlotMs = -1.0;
	plot->replot();
}

void MainWin::receivedNewData() {
	titleUnsaved();
	plot->graph()->addData(
		data->getXData(lastCustomPlotMs),
		data->getYData(lastCustomPlotMs)
	);
	lastCustomPlotMs = data->getLastCustomPlotMs();
	plot->rescaleAxes();
	plot->replot();
}

void MainWin::closeEvent(QCloseEvent *event) {
	if (!data->isDataSaved()) {
		auto ans = QMessageBox::question(this, APP_NAME,
			"Wyjœæ bez zapisywania?",
			QMessageBox::Cancel ,
			QMessageBox::Yes);
		if (ans != QMessageBox::Yes) {
			event->ignore();
			return;
		}
	}
	event->accept();
}