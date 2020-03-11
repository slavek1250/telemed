#pragma once

#include <QObject>
#include <queue>
#include <QDateTime>
#include <iir/Butterworth.h>

auto constexpr TIMER_INTERVAL = 900;
auto constexpr FILTER_ORDER = 2;

auto constexpr SAMPLING_RATE = 100.0;	// Hz
auto constexpr LOW_CUT_FREQ = 0.7 / 0.5;
auto constexpr HIGH_CUT_FREQ = 3 / 0.5;

class QTimer;
class QNetworkReply;
class QNetworkAccessManager;

class Data : public QObject
{
	Q_OBJECT
private:
	bool dataSaved = true;

	QTimer * timer;
	QNetworkAccessManager * manager;

	Iir::Butterworth::BandPass<FILTER_ORDER> filter;

	const QString MAIN_DATA_NAME = "HbO2";
	const QString BEAT_DATA_NAME = "Beat";

	QVector<qint64> xMainData;
	QVector<double> yMainData;
	QVector<qint64> xBeatData;
	QVector<double> yBeatData;

	struct ToCustomPlotMs{ 
		double operator()(qint64 time) {
			return time / 1000.0;
		}
	};
	struct FromCustomPlotMs {
		qint64 operator()(double ms) {
			return (ms * 1000 + 0.5);
		}
	};


	qint64 begMs = 0;

	int getRangeSize(qint64 begin, const QVector<qint64> & vec);
	void processNewData(QString && data_);

public:
	Data(QObject *parent);
	~Data();

	void start();
	void stop();
	void clear();
	void saveAs(const QString & filepath);
	QString getMainDataName();
	QString getBeatDataName();
	QVector<double> getXMainData(double dataLaterThan = -1.0);
	QVector<double> getYMainData(double dataLaterThan = -1.0);
	double getLastCustomPlotMsMainData();
	QVector<double> getXBeatData(double dataLaterThan = -1.0);
	QVector<double> getYBeatData(double dataLaterThan = -1.0);
	double getLastCustomPlotMsBeatData();

	//double getMaxForLast(double secs);
	//double getMinForLast(double secs);
	std::pair<double, double> getMinMaxForLast(double secs);

	bool isDataSaved() { return dataSaved; };

private slots:
	void timerTimeout();
	void networkResponse(QNetworkReply * reply);

public slots:

signals:
	void receivedNewData();
};
