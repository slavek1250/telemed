#include "Data.h"

#include <QTimer>

Data::Data(QObject *parent)
	: QObject(parent)
{
	timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &Data::timerTimeout);
	timer->setInterval(TIMER_INTERVAL);

	xData = QVector<double>();
	yData = QVector<double>();
}

Data::~Data()
{
}

void Data::timerTimeout() {

	for (int i = 0; i < 10; ++i) {
		xData.push_back(xData.size());
		yData.push_back(rand() % xData.size());
	}
	emit receivedNewData();
}

void Data::start() {
	timer->start();
}

void Data::stop() {
	timer->stop();
}

void Data::clear() {
	xData.clear();
	yData.clear();
}

void Data::saveAs(const QString & filepath)
{
}

QString Data::getDataName() {
	return DATA_NAME;
}

QVector<double> Data::getXData(int startFrom) {
	QVector<double> x(xData.size() - startFrom);
	std::copy(xData.begin() + startFrom, xData.end(), x.begin());
	return x;
}

QVector<double> Data::getYData(int startFrom) {
	QVector<double> y(yData.size() - startFrom);
	std::copy(yData.begin() + startFrom, yData.end(), y.begin());
	return y;
}

int Data::getLastId() {
	return xData.size() - 1;
}
