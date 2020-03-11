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

	ui.rangeLn->setValidator(new QRegExpValidator(QRegExp("[1-9]\\d*"), this));
	ui.rangeLn->clearFocus();

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
	plot->graph(Graph::MAIN)->setName(data->getMainDataName());
	plot->addGraph();
	plot->graph(Graph::BEAT)->setPen(QPen(Qt::red));
	//plot->graph(1)->setBrush(QBrush(QPixmap("media/arrow-down-icon.png")));
	plot->graph(Graph::BEAT)->setLineStyle(QCPGraph::lsNone);
	plot->graph(Graph::BEAT)->setScatterStyle(
		QCPScatterStyle(
			QPixmap("media/arrow-down-icon.png").scaled(30, 30)
	));
	plot->graph(Graph::BEAT)->setName(data->getBeatDataName());

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
			"Clear not saved data?",
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
	lastCustomPlotMsMainData = -1.0;
	lastCustomPlotMsBeatData = -1.0;
	plot->replot();
	ui.HRLbl->setText("");
	ui.HRTab->clearContents();
}

void MainWin::receivedNewData() {
	titleUnsaved();
	
	plot->graph(Graph::MAIN)->addData(
		data->getXMainData(lastCustomPlotMsMainData),
		data->getYMainData(lastCustomPlotMsMainData)
	);
	plot->graph(Graph::BEAT)->addData(
		data->getXBeatData(lastCustomPlotMsBeatData),
		data->getYBeatData(lastCustomPlotMsBeatData)
	);
	
	//plot->rescaleAxes();
	int range = ui.rangeLn->text().toInt();
	auto yMinMax = data->getMinMaxForLast(range);
	plot->yAxis->setRange(
		yMinMax.first,
		yMinMax.second
	);
	plot->xAxis->setRange(
		lastCustomPlotMsMainData - range,
		lastCustomPlotMsMainData
	);
	plot->replot();


	auto hrs = data->getHeartRate(lastHRMs);
	for (auto hr : hrs) {
		//ui.HRTab->
		QTableWidgetItem * begIt = new QTableWidgetItem(
			QDateTime::fromMSecsSinceEpoch(hr.begin).toString("hh:mm:ss.zzz")
		);
		QTableWidgetItem * endIt = new QTableWidgetItem(
			QDateTime::fromMSecsSinceEpoch(hr.end).toString("hh:mm:ss.zzz")
		);
		QTableWidgetItem * hrIt = new QTableWidgetItem(
			tr("%1").arg(hr.hr)
		);

		auto row = ui.HRTab->rowCount();
		ui.HRTab->setRowCount(row + 1);
		ui.HRTab->setItem(row, 0, begIt);
		ui.HRTab->setItem(row, 1, endIt);
		ui.HRTab->setItem(row, 2, hrIt);

	}
	if (!hrs.isEmpty()) {
		ui.HRLbl->setText(QString::number(hrs.back().hr));
		lastHRMs = hrs.back().begin;
	}

	QScrollBar *sb = ui.HRTab->verticalScrollBar();
	sb->setValue(sb->maximum());
	

	lastCustomPlotMsMainData = data->getLastCustomPlotMsMainData();
	lastCustomPlotMsBeatData = data->getLastCustomPlotMsBeatData();
}

void MainWin::closeEvent(QCloseEvent *event) {
	if (!data->isDataSaved()) {
		auto ans = QMessageBox::question(this, APP_NAME,
			"Exit program without saving?",
			QMessageBox::Cancel ,
			QMessageBox::Yes);
		if (ans != QMessageBox::Yes) {
			event->ignore();
			return;
		}
	}
	event->accept();
}