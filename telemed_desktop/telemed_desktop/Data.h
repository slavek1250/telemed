#pragma once

#include <QObject>
#include <set>
#include <vector>
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
	const QString HEART_DATA_NAME = "Heart Rate";
	bool irDataEnabled = true;
	bool redDataEnabled = true;
	bool hrDataEnabled = true;

	std::set<SensorData> sensorDataSet;
	std::set<qint64> beatSet;
	std::vector<HeartRate> heartRateVecRaw;
	std::vector<HeartRate> heartRateVec;
	unsigned int quantileMeanN = 10;

	qint64 begMs = 0;

	QVector<double> getSensorData(
		double dataLaterThan, 
		const std::function<double(const SensorData &)> & fMap);
	std::set<SensorData>::iterator getRangeBegin(double laterThanCustomPlotMs);
	std::set<SensorData>::iterator getRangeBegin(qint64 laterThanMs);
	std::pair<double, double> sensorDataMinMax(
		const std::set<SensorData>::iterator & begin,
		const std::set<SensorData>::iterator & end);
	std::vector<HeartRate>::iterator getHeartRateBegin(qint64 laterThanMs);
	std::pair<double, double> heartRateMinMax(
		const std::vector<HeartRate>::iterator & begin,
		const std::vector<HeartRate>::iterator & end
	);
	template<class T>
	double quantileMean(
		const typename std::vector<T>::iterator & begin,
		const typename std::vector<T>::iterator & end
	);
	template<class T, class Functor>
	double quantileMean(
		const typename std::vector<T>::iterator & begin,
		const typename std::vector<T>::iterator & end,
		const Functor & mapF
	);
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
	void setHearRateEnabled(bool enabled);

	QString getYIrSensorDataName() const;
	QString getYRedSensorDataName() const;
	QVector<double> getXSensorData(double dataLaterThan = -1.0);
	QVector<double> getYIrSensorData(double dataLaterThan = -1.0);
	QVector<double> getYRedSensorData(double dataLaterThan = -1.0);
	double getLastSensorDataCustomPlotMs();
	std::pair<double, double> getDataMinMax(int rangeSizeInSeconds);

	QString getBeatDataName() const;
	QVector<double> getXBeatData();
	QVector<double> getYBeatData(int rangeSizeInSeconds);

	void setHeartRateQuantileN(unsigned int n);
	QString getHeartRateDataName() const;
	QVector<HeartRate> getHeartRate(qint64 laterThan = -1);
	//QVector<HeartRate> getMeanHeartRate(qint64 laterThan = -1, unsigned int meanForN = 10);
	QVector<HeartRate> getQuantileMeanHeartRate(qint64 laterThan = -1, unsigned int quantileForN = 10);
	QVector<double> getXHRData() const;
	QVector<double> getYHRData() const;

	bool isDataSaved() { return dataSaved; };

private slots:
	void timerTimeout();
	void processNewData(const QString & data_);
	void detectHeartRate(std::set<SensorData>::iterator begin);

signals:
	void receivedNewData();
};

template<class T> inline double Data::quantileMean(
	const typename std::vector<T>::iterator & begin, 
	const typename std::vector<T>::iterator & end) 
{
	return quantileMean(begin, end, [](const T & v)->double { return double(v); });
}

template<class T, class Functor> inline double Data::quantileMean(
	const typename std::vector<T>::iterator & begin,
	const typename std::vector<T>::iterator & end,
	const Functor & mapF)
{
	std::vector<double> vals(std::distance(begin, end));
	std::transform(begin, end, vals.begin(), mapF);
	std::sort(vals.begin(), vals.end());
	int beginIndex = vals.size() * 0.3; //> 3 ? 1 : 0;
	int endIndex = vals.size() - beginIndex;
	double sum = std::accumulate(
		vals.begin() + beginIndex,
		vals.begin() + endIndex, 0.0);
	return sum / (endIndex - beginIndex);
}
