#include "Data.h"

#include <nlohmann/json.h>
#include <QDateTime>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <iir/Butterworth.h>
#include "ObjectFactory.h"
#include "DeviceApi.h"
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

	xMainData = QVector<qint64>();
	yMainData = QVector<double>();
	xBeatData = QVector<qint64>();
	yBeatData = QVector<double>();
	

	filter.setup(
		SAMPLING_RATE,
		(LOW_CUT_FREQ + HIGH_CUT_FREQ) / 2,
		(HIGH_CUT_FREQ - LOW_CUT_FREQ)
	);
	
	auto devApi = ObjectFactory::getInstance<DeviceApi>();
	connect(devApi, &DeviceApi::newMeasuresData, this, &Data::processNewData);
}

Data::~Data()
{
}

void Data::timerTimeout() {
	dataSaved = false;
	auto devApi = ObjectFactory::getInstance<DeviceApi>();
	devApi->readNewMeasures();
}

void Data::start() {
	timer->start();
}

void Data::stop() {
	timer->stop();
}

void Data::clear() {
	xMainData.clear();	
	yMainData.clear();
	xBeatData.clear();
	yBeatData.clear();
	dataSaved = true;
}

void Data::saveAs(const QString & filepath) {
	QFile file(filepath);

	if (file.open(QIODevice::WriteOnly)) {
		QTextStream ts(&file);
		ts << "Time [ms];" << MAIN_DATA_NAME;
		for (int i = 0; i < xMainData.size(); ++i) {
			ts << "\n" << (xMainData.at(i) - xMainData.front()) << ";" << yMainData.at(i);
			//if(i != (xMainData.size() - 1)) ts << "\n";
		}
		dataSaved = true;
	}
	file.close();
}

QString Data::getMainDataName() {
	return MAIN_DATA_NAME;
}

QString Data::getBeatDataName() {
	return BEAT_DATA_NAME;
}

QVector<double> Data::getXMainData(double dataLaterThan) {
	qint64 beg = FromCustomPlotMs()(dataLaterThan);
	auto size = (dataLaterThan == -1.0) ? xMainData.size() : getRangeSize(beg, xMainData) - 1;
	QVector<double> x(size);
	std::transform(
		xMainData.end() - size, 
		xMainData.end(), 
		x.begin(), 
		ToCustomPlotMs()
	);
	return x;
}

QVector<double> Data::getYMainData(double dataLaterThan) {
	qint64 beg = FromCustomPlotMs()(dataLaterThan);
	auto size = (dataLaterThan==-1.0) ? yMainData.size() : getRangeSize(beg, xMainData) - 1;
	QVector<double> y(size);
	std::copy(yMainData.end() - size, yMainData.end(), y.begin());
	return y;
}

double Data::getLastCustomPlotMsMainData() {
	return ToCustomPlotMs()(xMainData.back());
}

QVector<double> Data::getXBeatData(double dataLaterThan) {
	qint64 beg = FromCustomPlotMs()(dataLaterThan);
	auto size = (dataLaterThan == -1.0) ? xBeatData.size() : getRangeSize(beg, xBeatData) - 1;
	QVector<double> x(size);
	std::transform(
		xBeatData.end() - size,
		xBeatData.end(),
		x.begin(),
		ToCustomPlotMs()
	);
	return x;
}

QVector<double> Data::getYBeatData(double dataLaterThan) {
	qint64 beg = FromCustomPlotMs()(dataLaterThan);
	auto size = (dataLaterThan == -1.0) ? yBeatData.size() : getRangeSize(beg, xBeatData) - 1;
	QVector<double> y(size);
	std::copy(yBeatData.end() - size, yBeatData.end(), y.begin());
	return y;
}

double Data::getLastCustomPlotMsBeatData() {
	return xBeatData.isEmpty() ? -1.0 : ToCustomPlotMs()(xBeatData.back());
}

QVector<HeartRate> Data::getHeartRate(qint64 laterThan) {
	auto it{
		laterThan == -1.0 ? 
		heartRateVec.begin() :
		std::find_if(heartRateVec.begin(), heartRateVec.end(),
			[laterThan](const HeartRate & hr)->bool {
				return hr.begin > laterThan;
	})};
	QVector<HeartRate> tmp(std::distance(it, heartRateVec.end()));
	std::copy(it, heartRateVec.end(), tmp.begin());
	return tmp;
}

std::pair<double, double> Data::getMinMaxForLast(double secs) {
	auto minMaxMain = minMaxInOneSerie(xMainData, yMainData, secs);
	auto minMaxBeat = minMaxInOneSerie(xBeatData, yBeatData, secs);
	
	return std::pair<double, double>(
		std::min(minMaxBeat.first, minMaxMain.first),
		std::max(minMaxBeat.second, minMaxMain.second)
	);
}

int Data::getRangeSize(qint64 begin, const QVector<qint64> & vec) {
	auto beg = std::find(vec.begin(), vec.end(), begin);
	return std::distance(beg, vec.end());
}

void Data::processNewData(const QString & data_) {
	nlohmann::json data = nlohmann::json::parse(data_.toStdString());

	// set begin time of measures
	if (begMs == 0) {
		auto max = std::max_element(data.begin(), data.end(), 
			[](const nlohmann::json::value_type & l, const nlohmann::json::value_type & r)->bool {
				return l["ms"].get<unsigned long long>() < r["ms"].get<unsigned long long>();
		});
		begMs = QDateTime::currentDateTime().toMSecsSinceEpoch() - (*max)["ms"].get<unsigned long long>();
	}
	
	std::vector<std::pair<qint64, int>> vals(data.size());

	// map json to vector, normalize time and filter signal
	std::transform( data.begin(), data.end(), vals.begin(), 
		[this](const nlohmann::json::value_type & row)->std::pair<qint64, int> {
			return std::pair<qint64, int>(
				(row["ms"].get<unsigned long long>() + this->begMs),
				this->filter.filter(row["red"].get<int>())
			);
	});

	// sort values
	std::sort(vals.begin(), vals.end(), 
		[](const std::pair<qint64, int> & l, const std::pair<qint64, int> & r)->bool {
			return l.first < r.first;
	});

	qint64 currNewestVal = (xMainData.isEmpty() ? 0 : xMainData.back());
	auto it = std::find_if(vals.begin(), vals.end(), 
		[currNewestVal](const std::pair<qint64, int> & row)->bool {
			return row.first > currNewestVal;
	});
	vals.erase(vals.begin(), it);

	std::transform(vals.begin(), vals.end(), std::back_inserter(xMainData), 
		[](const std::pair<qint64, int> & row)->qint64 {
			return row.first;
	});
	std::transform(vals.begin(), vals.end(), std::back_inserter(yMainData),
		[](const std::pair<qint64, int> & row)->double {
			return row.second;
	});

	/*

	double mean = std::accumulate(vals.begin(), vals.end(), 0,
		[](int accumulated, const std::pair<qint32, int> & val)->int {
			return accumulated + val.second;
	});
	mean /= vals.size();

	auto newEnd = std::remove_if(vals.begin(), vals.end(),
		[mean](const std::pair<qint64, int> & val)->bool {
			return (val.second < 2 * mean);
	});

	vals.erase(newEnd, vals.end());

	std::transform(vals.begin(), vals.end(), std::back_inserter(xBeatData),
		[](const std::pair<qint64, int> & row)->qint64 {
		return row.first;
	});
	std::transform(vals.begin(), vals.end(), std::back_inserter(yBeatData),
		[](const std::pair<qint64, int> & row)->double {
		return row.second;
	});

	*/
	
	for (auto & sample : vals) {
		if (beatDetector.addSample(sample.first, sample.second)) {
			xBeatData.push_back(sample.first);
			yBeatData.push_back(sample.second + 10);

			if (xBeatData.size() > 1) {
				auto lastMs = xBeatData.at(xBeatData.size() - 2);
				auto currMs = xBeatData.back();
				heartRateVec.push_back(HeartRate{
					lastMs, currMs, 60000.0 / (currMs - lastMs)
				});
			}
		}
	}
	
	emit receivedNewData();
}

std::pair<double, double> Data::minMaxInOneSerie(const QVector<qint64>& x, const QVector<double>& y, double range) {
	std::pair<double, double> minMax(0, 5);
	if (!x.isEmpty()) {
		qint64 beg = x.back() - FromCustomPlotMs()(range);
		auto it = std::find_if(x.begin(), x.end(),
			[beg](qint64 t)->bool {
			return t > beg;
		});
		auto mainBegId = std::distance(x.begin(), it);
		auto tmp = std::minmax_element(y.begin() + mainBegId, y.end());
		minMax = std::pair<double, double>(*tmp.first, *tmp.second);
	}
	return minMax;
}

