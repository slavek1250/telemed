#pragma once

#include <QObject>
#include <set>
#include <functional>
#include <iir/Butterworth.h>
#include "MAX30100_BeatDetector.h"
#include "HeartRate.h"
#include "SensorData.h"

auto constexpr TIMER_INTERVAL = 1000;
auto constexpr FILTER_ORDER = 2;

auto constexpr SAMPLING_RATE = 100.0;	// Hz
auto constexpr LOW_CUT_FREQ = 1.4;		// / 0.5;
auto constexpr HIGH_CUT_FREQ = 6;		// / 0.5;

class QTimer;

class Data : public QObject
{
	Q_OBJECT
private:
	bool dataSaved = true;

	QTimer * timer;

	Iir::Butterworth::BandPass<FILTER_ORDER> redFilter;
	Iir::Butterworth::BandPass<FILTER_ORDER> irFilter;
	BeatDetector beatDetector;

	const QString IR_DATA_NAME = "IR led";
	const QString RED_DATA_NAME = "Red led";
	const QString BEAT_DATA_NAME = "Beat";
	bool irDataEnabled = true;
	bool redDataEnabled = true;

	std::set<SensorData> sensorDataSet;
	std::set<qint64> beatSet;
	std::set<HeartRate> heartRateSet;

	qint64 begMs = 0;

	QVector<double> getSensorData(
		double dataLaterThan, 
		const std::function<double(const SensorData &)> & fMap);
	std::set<SensorData>::iterator getRangeBegin(double laterThanCustomPlotMs);
	std::set<SensorData>::iterator getRangeBegin(qint64 laterThanMs);
	std::pair<double, double> sensorDataMinMax(
		const std::set<SensorData>::iterator & begin,
		const std::set<SensorData>::iterator & end);
	static double msToCustomPlotMs(qint64 ms);
	static qint64 customPlotMsToMs(double cMs);
	static std::string timestampStringFromMsSinceEpoch(qint64 ms);

public:
	Data(QObject *parent);
	~Data() {};

	void start();
	void stop();
	void clear();
	void saveAs(const QString & filepath);

	void setIrLedEnabled(bool enabled);
	void setRedLedEnabled(bool enabled);

	QString getYIrSensorDataName() const;
	QString getYRedSensorDataName() const;
	QVector<double> getXSensorData(double dataLaterThan = -1.0);
	QVector<double> getYIrSensorData(double dataLaterThan = -1.0);
	QVector<double> getYRedSensorData(double dataLaterThan = -1.0);
	double getLastSensorDataCustomPlotMs();
	std::pair<double, double> getSensorDataMinMax(int rangeSizeInSeconds);

	QString getBeatDataName() const;
	QVector<double> getXBeatData();
	QVector<double> getYBeatData(int rangeSizeInSeconds);

	QVector<HeartRate> getHeartRate(qint64 laterThan = -1);

	bool isDataSaved() { return dataSaved; };

private slots:
	void timerTimeout();
	void processNewData(const QString & data_);

signals:
	void receivedNewData();
};