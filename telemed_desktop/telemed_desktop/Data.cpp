#include "Data.h"

#include <QDateTime>
#include <QTimer>

/*
	ESP sends:
		{
			data:[
				{
					T:uint32_t // ms since device started
					V:uint16_t // value
				}, ...
			]
		}
*/


Data::Data(QObject *parent)
	: QObject(parent)
{
	timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &Data::timerTimeout);
	timer->setInterval(TIMER_INTERVAL);

	xData = QVector<QDateTime>();
	yData = QVector<double>();
}

Data::~Data()
{
}

void Data::timerTimeout() {
	dataSaved = false;

	for (int i = 0; i < 100; ++i) {
		xData.push_back(QDateTime::fromMSecsSinceEpoch(xData.size() * 10 + begMs));
		yData.push_back(std::sin(xData.size() * 2 * 3.14 / 100));
	}
	emit receivedNewData();
}

void Data::start() {
	timer->start();
	begMs = QDateTime::currentDateTime().toMSecsSinceEpoch();
}

void Data::stop() {
	timer->stop();
}

void Data::clear() {
	xData.clear();	
	yData.clear();
	dataSaved = true;
}

void Data::saveAs(const QString & filepath) {

	dataSaved = true;
}

QString Data::getDataName() {
	return DATA_NAME;
}

QVector<double> Data::getXData(double dataLaterThan) {
	auto size = (dataLaterThan == -1.0) ? yData.size() : getRangeSize(dataLaterThan) - 1;
	QVector<double> x(size);
	std::transform(
		xData.end() - size, 
		xData.end(), 
		x.begin(), 
		dateTimeToCustomPlotMs()
	);
	return x;
}

QVector<double> Data::getYData(double dataLaterThan) {
	auto size = (dataLaterThan==-1.0) ? yData.size() : getRangeSize(dataLaterThan) - 1;
	QVector<double> y(size);
	std::copy(yData.end() - size, yData.end(), y.begin());
	return y;
}

int Data::getLastId() {
	return xData.size() - 1;
}

double Data::getLastCustomPlotMs() {
	return dateTimeToCustomPlotMs()(xData.back());
}


int Data::getRangeSize(double begin) {
	auto begDateTime = customPlotMsToDateTime()(begin);
	auto beg = std::find(xData.begin(), xData.end(), begDateTime);
	return std::distance(beg, xData.end());
}