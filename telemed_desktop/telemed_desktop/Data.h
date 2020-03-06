#pragma once

#include <QObject>
#include <queue>

auto constexpr TIMER_INTERVAL = 1000;

class QTimer;

class Data : public QObject
{
	Q_OBJECT
private:
	QTimer * timer;
	const QString DATA_NAME = "data";
	std::vector<std::pair<double, double>> data;
	QVector<double> xData, yData;

public:
	Data(QObject *parent);
	~Data();

	void start();
	void stop();
	void clear();
	void saveAs(const QString & filepath);
	QString getDataName();
	//std::vector<std::pair<double, double>> getData(int startFrom);
	QVector<double> getXData(int startFrom = 0);
	QVector<double> getYData(int startFrom = 0);
	int getLastId();


private slots:
	void timerTimeout();

public slots:

signals:
	void receivedNewData();
};
