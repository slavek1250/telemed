#include "Data.h"

#include <nlohmann/json.h>
#include <QDateTime>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <iir/Butterworth.h>
#include "ObjectFactory.h"
#include "DeviceApi.h"


Data::Data(QObject *parent)
	: QObject(parent)
{
	timer = new QTimer(this);
	auto devApi = ObjectFactory::getInstance<DeviceApi>();
	
	connect(devApi, &DeviceApi::newMeasuresData, this, &Data::processNewData);
	connect(timer, &QTimer::timeout, this, &Data::timerTimeout);
	timer->setInterval(TIMER_INTERVAL);

	filter.setup(
		SAMPLING_RATE,
		(LOW_CUT_FREQ + HIGH_CUT_FREQ) / 2,
		(HIGH_CUT_FREQ - LOW_CUT_FREQ)
	);	
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
	sensorDataSet.clear();
	beatSet.clear();
	heartRateSet.clear();
	begMs = 0;
	dataSaved = true;
}

void Data::saveAs(const QString & filepath) {
	QFile file(filepath);

	if (file.open(QIODevice::WriteOnly)) {
		QTextStream ts(&file);
		ts << "Time [ms];" << IR_DATA_NAME << RED_DATA_NAME;
		for(auto & sd : sensorDataSet) {
			ts << "\n"
				<< (sd.getMs() - (*sensorDataSet.begin()).getMs()) << ";"
				<< sd.getIrLed() << ";" << sd.getRedLed();
		}
		dataSaved = true;
	}
	file.close();
}

void Data::setIrLedEnabled(bool enabled) {
	irDataEnabled = enabled;
}

void Data::setRedLedEnabled(bool enabled) {
	redDataEnabled = enabled;
}

QString Data::getYIrSensorDataName() const {
	return IR_DATA_NAME;
}

QString Data::getYRedSensorDataName() const {
	return RED_DATA_NAME;
}

QVector<double> Data::getXSensorData(double dataLaterThan) {
	return getSensorData(dataLaterThan, [](const SensorData & data)->double {
		return data.toCustomPlotMs();
	});
}

QVector<double> Data::getYIrSensorData(double dataLaterThan) {
	return getSensorData(dataLaterThan, [](const SensorData & data)->double {
		return data.getIrLed();
	});
}

QVector<double> Data::getYRedSensorData(double dataLaterThan) {
	return getSensorData(dataLaterThan, [](const SensorData & data)->double {
		return data.getRedLed();
	});
}

double Data::getLastSensorDataCustomPlotMs() {
	return (*sensorDataSet.rbegin()).toCustomPlotMs();
}

std::pair<double, double> Data::getSensorDataMinMax(int rangeSizeInSeconds) {
	if (sensorDataSet.empty()) return std::pair<double, double>(0, 1);
	auto begin = getRangeBegin((*sensorDataSet.rbegin()).getMs() - rangeSizeInSeconds * 1000);
	if (begin == sensorDataSet.end())
		begin = sensorDataSet.begin();
	return sensorDataMinMax(begin, sensorDataSet.end());
}

QString Data::getBeatDataName() const {
	return BEAT_DATA_NAME;
}

QVector<double> Data::getXBeatData() {
	QVector<double> x(beatSet.size());
	std::transform(beatSet.begin(), beatSet.end(), x.begin(), [this](qint64 ms)->double {
		return msToCustomPlotMs(ms);
	});
	return x;
}

QVector<double> Data::getYBeatData(int rangeSizeInSeconds) {
	auto begin = getRangeBegin((*sensorDataSet.rbegin()).getMs() - rangeSizeInSeconds * 1000);
	auto max = sensorDataMinMax(begin, sensorDataSet.end()).second;
	return QVector<double>(beatSet.size(), max);
}

QVector<HeartRate> Data::getHeartRate(qint64 laterThan) {
	auto begin = std::find_if(heartRateSet.begin(), heartRateSet.end(), 
		[laterThan](const HeartRate & hr)->bool {
			return hr.getBeginMs() > laterThan;
	});
	QVector<HeartRate> hr(std::distance(begin, heartRateSet.end()));
	std::copy(begin, heartRateSet.end(), hr.begin());
	return hr;
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
	
	auto previousLastMs = sensorDataSet.empty() ? 
		-1 : (*sensorDataSet.rbegin()).getMs();
	
	std::transform(data.begin(), data.end(), std::inserter(sensorDataSet, sensorDataSet.begin()),
		[this](const nlohmann::json::value_type & row)->SensorData {
			return SensorData(
				(row["ms"].get<unsigned long long>() + this->begMs),
				row["ir"].get<int>(),
				this->filter.filter(row["red"].get<int>())
			);	
	});

	auto it = previousLastMs == -1 ? 
		sensorDataSet.begin() : sensorDataSet.find(SensorData(previousLastMs));
	for (; it != sensorDataSet.end(); ++it) {
		auto sd = *it;
		if (beatDetector.addSample(sd.getMs(), sd.getRedLed())) {
			if (!beatSet.empty()) {
				heartRateSet.insert(HeartRate(
					(*beatSet.rbegin()),	// begin
					sd.getMs()				// end
				));
			}
			beatSet.insert(sd.getMs());
		}
	}
	emit receivedNewData();
}

QVector<double> Data::getSensorData(double dataLaterThan, const std::function<double(const SensorData&)> & fMap) {
	auto begin = getRangeBegin(dataLaterThan);
	auto size = std::distance(begin, sensorDataSet.end());
	QVector<double> data(size);
	std::transform(begin, sensorDataSet.end(), data.begin(), fMap);
	return data;
}

std::set<SensorData>::iterator Data::getRangeBegin(double laterThanCustomPlotMs) {
	return getRangeBegin(customPlotMsToMs(laterThanCustomPlotMs));
}

std::set<SensorData>::iterator Data::getRangeBegin(qint64 laterThanMs) {
	return std::find_if(sensorDataSet.begin(), sensorDataSet.end(),
		[laterThanMs](const SensorData & data)->bool {
			return data.getMs() > laterThanMs;
	});
}

std::pair<double, double> Data::sensorDataMinMax(
	const std::set<SensorData>::iterator & begin,
	const std::set<SensorData>::iterator & end)
{
	if (std::distance(begin, end) == 0 || (!irDataEnabled && !redDataEnabled))
		return std::pair<double, double>(0, 1);

	std::set<int> tmp;
	if (irDataEnabled) {
		std::transform(begin, end, std::inserter(tmp, tmp.begin()),
			[](const SensorData & data)->int {
			return data.getIrLed();
		});
	}
	if (redDataEnabled) {
		std::transform(begin, end, std::inserter(tmp, tmp.begin()),
			[](const SensorData & data)->int {
			return data.getRedLed();
		});
	}
	return std::pair<double, double>(*tmp.begin(), *tmp.rbegin());
}

double Data::msToCustomPlotMs(qint64 ms) {
	return ms / 1000.0;
}

qint64 Data::customPlotMsToMs(double cMs) {
	return cMs * 1000 + 0.5;
}
