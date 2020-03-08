#pragma once

#include <QObject>
#include <queue>
#include <QDateTime>

auto constexpr TIMER_INTERVAL = 1000;

class QTimer;

class Data : public QObject
{
	Q_OBJECT
private:
	bool dataSaved = true;

	QTimer * timer;
	const QString DATA_NAME = "data";
	std::vector<std::pair<double, double>> data;
	
	QVector<double> yData;
	QVector<QDateTime> xData;

	struct dateTimeToCustomPlotMs{ 
		double operator()(const QDateTime & time) {
			return time.toMSecsSinceEpoch() / 1000.0;
		}
	};
	struct customPlotMsToDateTime {
		QDateTime operator()(double ms) {
			return QDateTime::fromMSecsSinceEpoch((ms * 1000 + 0.5));
		}
	};
	qint64 begMs;

	int getRangeSize(double begin);

public:
	Data(QObject *parent);
	~Data();

	void start();
	void stop();
	void clear();
	void saveAs(const QString & filepath);
	QString getDataName();
	//std::vector<std::pair<double, double>> getData(int startFrom);
	QVector<double> getXData(double dataLaterThan = -1.0);
	QVector<double> getYData(double dataLaterThan = -1.0);
	int getLastId();
	double getLastCustomPlotMs();

	bool isDataSaved() { return dataSaved; };

private slots:
	void timerTimeout();

public slots:

signals:
	void receivedNewData();
};
