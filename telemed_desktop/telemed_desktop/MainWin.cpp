#include "MainWin.h"
#include <QCloseEvent>
#include <QFileDialog>
#include <QStandardPaths>
#include <QCustomPlot.h>
#include "Data.h"
#include "ObjectFactory.h"
#include "DeviceApi.h"

MainWin::MainWin(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	ObjectFactory::createInstance(new DeviceApi(this));
	data = new Data(this);
	plot = new QCustomPlot(this);
	auto devApi = ObjectFactory::getInstance<DeviceApi>();

	titleSaved();
	//ui.tableDockWgt->setVisible(false);
	//ui.propDockWgt->setVisible(false);
	ui.plotLayout->addWidget(plot);
	setupPlot();

	ui.rangeLn->setValidator(new QRegExpValidator(
		QRegExp("[1-9]\\d*"), this
	));
	ui.ipEdt->setValidator(new QRegExpValidator(
		QRegExp("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}"), this
	));
	
	devApi->setDeviceIp(ui.ipEdt->text());

	connect(ui.actionStartStop, &QAction::toggled, this, &MainWin::startStop);
	connect(ui.actionSaveAs, &QAction::triggered, this, &MainWin::saveToFile);
	connect(ui.actionClear, &QAction::triggered, this, &MainWin::clear);
	connect(data, &Data::receivedNewData, this, &MainWin::receivedNewData);
	connect(ui.ipEdt, &QLineEdit::textChanged, devApi, &DeviceApi::setDeviceIp);
	connect(ui.irLedCurrBox, qOverload<int>(&QComboBox::currentIndexChanged), 
		devApi, &DeviceApi::setIrLedCurrent);
	connect(ui.redLedCurrBox, qOverload<int>(&QComboBox::currentIndexChanged),
		devApi, &DeviceApi::setRedLedCurrent);
	connect(ui.redChckBox, &QCheckBox::toggled, this, &MainWin::setRedLedGraphVisible);
	connect(ui.irChckBox, &QCheckBox::toggled, this, &MainWin::setIrLedGraphVisible);
	connect(ui.beatChckBox, &QCheckBox::toggled, this, &MainWin::setBeatGraphVisible);
	connect(ui.rangeLn, &QLineEdit::editingFinished, this, &MainWin::updateRange);
}

void MainWin::startStop(bool toggled) {
	auto devApi = ObjectFactory::getInstance<DeviceApi>();
	ui.actionStartStop->setText(toggled ? "Start" : "Stop");
	if (toggled) {
		data->stop();
		devApi->setIrLedCurrent(0);
		devApi->setRedLedCurrent(0);
	}
	else {
		data->start();
		devApi->setIrLedCurrent(ui.irLedCurrBox->currentIndex());
		devApi->setRedLedCurrent(ui.redLedCurrBox->currentIndex());
	}
}

void MainWin::setupPlot() {
	plot->addGraph();
	plot->graph(Graph::IR)->setPen(QPen(Qt::blue));
	plot->graph(Graph::IR)->setName(data->getYIrSensorDataName());
	ui.irChckBox->setText(data->getYIrSensorDataName());
	plot->addGraph();
	plot->graph(Graph::RED)->setPen(QPen(Qt::red));
	plot->graph(Graph::RED)->setName(data->getYRedSensorDataName());
	ui.redChckBox->setText(data->getYRedSensorDataName());
	plot->addGraph();
	plot->graph(Graph::BEAT)->setLineStyle(QCPGraph::lsNone);
	plot->graph(Graph::BEAT)->setScatterStyle(
		QCPScatterStyle(
			QPixmap("media/arrow-down-icon.png").scaled(30, 30)
	));
	plot->graph(Graph::BEAT)->setName(data->getBeatDataName());
	ui.beatChckBox->setText(data->getBeatDataName());

	plot->legend->setVisible(true);
	plot->legend->setBrush(QColor(255, 255, 255, 150));
	QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
	dateTicker->setDateTimeFormat("hh:mm:ss.zzz");
	plot->xAxis->setTicker(dateTicker);
}

void MainWin::titleUnsaved() {
	this->setWindowTitle(APP_NAME + "*");
}

void MainWin::titleSaved() {
	this->setWindowTitle(APP_NAME);
}

void MainWin::setGraphVisible(Graph graph, bool visible) {
	plot->graph(graph)->setVisible(visible);
	if (visible) {
		plot->graph(graph)->addToLegend();
	}
	else {
		plot->graph(graph)->removeFromLegend();
	}
	updateRange();
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
	plot->replot();
	ui.HRLbl->setText("");
	ui.HRTab->clearContents();
	ui.HRTab->setRowCount(0);
}

void MainWin::receivedNewData() {
	titleUnsaved();
	auto tmp = data->getYIrSensorData(lastCustomPlotMsMainData);
	plot->graph(Graph::IR)->addData(
		data->getXSensorData(lastCustomPlotMsMainData),
		tmp
	);
	plot->graph(Graph::RED)->addData(
		data->getXSensorData(lastCustomPlotMsMainData),
		data->getYRedSensorData(lastCustomPlotMsMainData)
	);
	plot->graph(Graph::BEAT)->setData(
		data->getXBeatData(),
		data->getYBeatData(ui.rangeLn->text().toInt())
	);
	lastCustomPlotMsMainData = data->getLastSensorDataCustomPlotMs();
	updateRange();
	
	auto hrs = data->getHeartRate(lastHRMs);
	for (auto hr : hrs) {
		//ui.HRTab->
		QTableWidgetItem * begIt = new QTableWidgetItem(
			hr.getBeginTimeStr());
		QTableWidgetItem * endIt = new QTableWidgetItem(
			hr.getEndTimeStr());
		QTableWidgetItem * hrIt = new QTableWidgetItem(
			hr.getHRStr());
		auto row = ui.HRTab->rowCount();
		ui.HRTab->setRowCount(row + 1);
		ui.HRTab->setItem(row, 0, begIt);
		ui.HRTab->setItem(row, 1, endIt);
		ui.HRTab->setItem(row, 2, hrIt);

	}
	if (!hrs.isEmpty()) {
		ui.HRLbl->setText(QString::number(hrs.back().getHR()));
		lastHRMs = hrs.back().getBeginMs();
	}

	QScrollBar *sb = ui.HRTab->verticalScrollBar();
	sb->setValue(sb->maximum());
}

void MainWin::setRedLedGraphVisible(bool visible) {
	setGraphVisible(Graph::RED, visible);
	data->setRedLedEnabled(visible);
}

void MainWin::setIrLedGraphVisible(bool visible) {
	setGraphVisible(Graph::IR, visible);
	data->setIrLedEnabled(visible);
}

void MainWin::setBeatGraphVisible(bool visible) {
	setGraphVisible(Graph::BEAT, visible);
}

void MainWin::updateRange() {
	int range = ui.rangeLn->text().toInt();
	auto yMinMax = data->getSensorDataMinMax(range);
	double div = std::pow(10, std::floor(std::log10(yMinMax.second - yMinMax.first)));
	double min = std::floor(yMinMax.first / div - 1) * div;
	double max = std::ceil(yMinMax.second / div + 1) * div;
	plot->yAxis->setRange(min, max);
	plot->xAxis->setRange(
		lastCustomPlotMsMainData - range,
		lastCustomPlotMsMainData
	);
	plot->replot();
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
	auto devApi = ObjectFactory::getInstance<DeviceApi>();
	devApi->setIrLedCurrent(0);
	devApi->setRedLedCurrent(0);
	event->accept();
}