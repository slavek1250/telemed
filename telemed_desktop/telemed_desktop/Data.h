#pragma once

#include <QObject>
#include <queue>
#include <QMap>
#include <QDateTime>
#include <iir/Butterworth.h>
#include "MAX30100_BeatDetector.h"

auto constexpr TIMER_INTERVAL = 900;
auto constexpr FILTER_ORDER = 2;

auto constexpr SAMPLING_RATE = 100.0;	// Hz
auto constexpr LOW_CUT_FREQ = 0.7 / 0.5;
auto constexpr HIGH_CUT_FREQ = 3 / 0.5;

class QTimer;

struct HeartRate {
	qint64 begin = 0;
	qint64 end = 0;
	double hr = 0.0;

	HeartRate(qint64 begin_, qint64 end_, double hr_) :
		begin(begin_), end(end_), hr(hr_) {}
	HeartRate() {}
};
struct PlotPoint {
	qint64 x = 0;
	double y = 0.0;

	PlotPoint(qint64 x_, double y_) :
		x(x_), y(y_) {}
	PlotPoint() {}
};
struct ToCustomPlotMs {
	double operator()(qint64 time) {
		return time / 1000.0;
	}
};
struct FromCustomPlotMs {
	qint64 operator()(double ms) {
		return (ms * 1000 + 0.5);
	}
};



class Data : public QObject
{
	Q_OBJECT
private:
	bool dataSaved = true;

	QTimer * timer;

	Iir::Butterworth::BandPass<FILTER_ORDER> filter;
	BeatDetector beatDetector;

	const QString MAIN_DATA_NAME = "HbO2";
	const QString BEAT_DATA_NAME = "Beat";

	QMap<qint64, double> mainData;

	QVector<qint64> xMainData;
	QVector<double> yMainData;
	QVector<qint64> xBeatData;
	QVector<double> yBeatData;

	QVector<HeartRate> heartRateVec;

	qint64 begMs = 0;

	int getRangeSize(qint64 begin, const QVector<qint64> & vec);
	std::pair<double, double> minMaxInOneSerie(const QVector<qint64> & x, const QVector<double> & y, double range);

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

	QVector<HeartRate> getHeartRate(qint64 laterThan = -1);

	//double getMaxForLast(double secs);
	//double getMinForLast(double secs);
	std::pair<double, double> getMinMaxForLast(double secs);

	bool isDataSaved() { return dataSaved; };

private slots:
	void timerTimeout();
	void processNewData(const QString & data_);

public slots:

signals:
	void receivedNewData();
};
