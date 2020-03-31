#include "Data.h"

#include <nlohmann/json.h>
#include <OpenXLSX/OpenXLSX.h>
#include <QDateTime>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <iir/Butterworth.h>
#include <iterator>
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

	irFilter.setup(
		SAMPLING_RATE,
		(LOW_CUT_FREQ + HIGH_CUT_FREQ) / 2,
		(HIGH_CUT_FREQ - LOW_CUT_FREQ)
	);	
	redFilter.setup(
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
	heartRateVec.clear();
	begMs = 0;
	dataSaved = true;
}

void Data::saveAs(const QString & filepath) {
	using namespace OpenXLSX;
	XLDocument doc;

	doc.CreateDocument(filepath.toStdString());
	doc.Workbook().AddWorksheet("Raw data");
	auto wks = doc.Workbook().Worksheet("Raw data");
	wks.Cell("A1").Value() = "Timestamp";
	wks.Cell("B1").Value() = "Millis since begin";
	wks.Cell("C1").Value() = "IR led";
	wks.Cell("D1").Value() = "Red led";
	int row = 2;
	qint64 beginMs = (*sensorDataSet.begin()).getMs();
	for (auto & sd : sensorDataSet) {
		wks.Cell(row, 1).Value() = timestampStringFromMsSinceEpoch(sd.getMs());
		wks.Cell(row, 2).Value() = sd.getMs() - beginMs;
		wks.Cell(row, 3).Value() = sd.getIrLed();
		wks.Cell(row++, 4).Value() = sd.getRedLed();
	}

	doc.Workbook().AddWorksheet("Beats");
	wks = doc.Workbook().Worksheet("Beats");
	wks.Cell("A1").Value() = "Timestamp";
	wks.Cell("B1").Value() = "Millis since begin";
	row = 2;
	beginMs = *beatSet.begin();
	for (auto & beat : beatSet) {
		wks.Cell(row, 1).Value() = timestampStringFromMsSinceEpoch(beat);
		wks.Cell(row++, 2).Value() = beat - beginMs;
	}

	doc.Workbook().AddWorksheet("Heart rate");
	wks = doc.Workbook().Worksheet("Heart rate");
	wks.Cell("A1").Value() = "Begin time";
	wks.Cell("B1").Value() = "End time";
	wks.Cell("C1").Value() = "Begin millis";
	wks.Cell("D1").Value() = "End millis";
	wks.Cell("E1").Value() = "Heart Rate [bpm]";
	row = 2;
	beginMs = (*heartRateVec.begin()).getBeginMs();
	for (auto & hr : heartRateVec) {
		wks.Cell(row, 1).Value() = timestampStringFromMsSinceEpoch(hr.getBeginMs());
		wks.Cell(row, 2).Value() = timestampStringFromMsSinceEpoch(hr.getEndMs());
		wks.Cell(row, 3).Value() = hr.getBeginMs() - beginMs;
		wks.Cell(row, 4).Value() = hr.getEndMs() - beginMs;
		wks.Cell(row++, 5).Value() = hr.getHR();
	}
	doc.Workbook().DeleteSheet("Sheet1");
	doc.SaveDocument();
	dataSaved = true;
}

void Data::setIrLedEnabled(bool enabled) {
	irDataEnabled = enabled;
}

void Data::setRedLedEnabled(bool enabled) {
	redDataEnabled = enabled;
}

void Data::setHearRateEnabled(bool enabled) {
	hrDataEnabled = enabled;
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

std::pair<double, double> Data::getDataMinMax(int rangeSizeInSeconds) {
	if (sensorDataSet.empty()) 
		return std::pair<double, double>(0, 1);

	auto begin = getRangeBegin((*sensorDataSet.rbegin()).getMs() - rangeSizeInSeconds * 1000);
	if (begin == sensorDataSet.end())
		begin = sensorDataSet.begin();
	auto sensorMinMax = sensorDataMinMax(begin, sensorDataSet.end());
	if (hrDataEnabled == false || heartRateVec.empty()) 
		return sensorMinMax;
	
	auto hrBegin = getHeartRateBegin(heartRateVec.back().getEndMs() - rangeSizeInSeconds * 1000);
	if (hrBegin == heartRateVec.end())
		hrBegin = heartRateVec.begin();
	auto hrMinMax = heartRateMinMax(hrBegin, heartRateVec.end());

	return std::pair<double, double>(
		std::min(sensorMinMax.first, hrMinMax.first),
		std::max(sensorMinMax.second, hrMinMax.second)
	);
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

void Data::setHeartRateQuantileN(unsigned int n) {
	quantileMeanN = n;
}

QString Data::getHeartRateDataName() const {
	return HEART_DATA_NAME;
}

QVector<HeartRate> Data::getHeartRate(qint64 laterThan) {
	auto begin = getHeartRateBegin(laterThan);
	QVector<HeartRate> hr(std::distance(begin, heartRateVec.end()));
	std::copy(begin, heartRateVec.end(), hr.begin());
	return hr;
}
/*
QVector<HeartRate> Data::getMeanHeartRate(qint64 laterThan, unsigned int meanForN) {
	QVector<HeartRate> hrVec(heartRateVec.size());
	double sum = 0.0;
	for (int i = 0, n = 0; i < heartRateVec.size(); ++i) {
		sum += heartRateVec.at(i).getHR();
		if (n != meanForN)
			++n;
		else {
			sum -= heartRateVec.at(i - meanForN).getHR();
		}
		hrVec[i] = HeartRate{
			heartRateVec.at(i).getBeginMs(),
			heartRateVec.at(i).getEndMs(),
			sum / n
		};
	}
	if (laterThan != -1) {
		auto newEnd = std::remove_if(hrVec.begin(), hrVec.end(), 
			[laterThan](const HeartRate & hr)->bool {
				return hr.getBeginMs() <= laterThan;
		});
		hrVec.erase(newEnd, hrVec.end());
	}
	return hrVec;
}
*/
QVector<HeartRate> Data::getQuantileMeanHeartRate(qint64 laterThan, unsigned int quantileForN) {
	auto begin = getHeartRateBegin(laterThan);
	QVector<HeartRate> out(std::distance(begin, heartRateVec.end()));
	std::copy(begin, heartRateVec.end(), out.begin());
	return out;

	/*
	auto begin = getHeartRateBegin(laterThan);
	auto beginIndex = std::distance(heartRateVec.begin(), begin);
	QVector<HeartRate> out(std::distance(begin, heartRateVec.end()));

	for (int i = beginIndex; i < heartRateVec.size(); ++i) {
		auto quantileBegin = heartRateVec.begin();
		if ((i - (int)quantileForN + 1) >= 0)
			std::advance(quantileBegin, i - quantileForN + 1);
		auto quantileEnd = heartRateVec.begin();
		std::advance(quantileEnd, i + 1);

		out[i - beginIndex] = HeartRate{
			heartRateVec.at(i).getBeginMs(),
			heartRateVec.at(i).getEndMs(),
			quantileMean<HeartRate>(quantileBegin, quantileEnd,
				[](const HeartRate & v)->double { return v.getHR(); }
			)
		};
	}
	return out;
	*/
}

QVector<double> Data::getXHRData() const {
	QVector<double> x(heartRateVec.size());
	std::transform(heartRateVec.begin(), heartRateVec.end(), x.begin(), 
		[](const HeartRate & hr)->double {
			return msToCustomPlotMs(hr.getEndMs());
	});
	return x;
}

QVector<double> Data::getYHRData() const {
	QVector<double> y(heartRateVec.size());
	std::transform(heartRateVec.begin(), heartRateVec.end(), y.begin(),
		[](const HeartRate & hr)->double {
		return hr.getHR();
	});
	return y;
}


void Data::processNewData(const QString & data_) {
	nlohmann::json data;
	try {
		data = nlohmann::json::parse(data_.toStdString());
	}
	catch (const std::exception & ex) {
		qDebug() << ex.what();
		return;
	}
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
				irFilter.filter(row["ir"].get<int>()),
				redFilter.filter(row["red"].get<int>())
			);	
	});

	auto it = previousLastMs == -1 ? 
		sensorDataSet.begin() : sensorDataSet.find(SensorData(previousLastMs));
	detectHeartRate(it);

	emit receivedNewData();
}

void Data::detectHeartRate(std::set<SensorData>::iterator begin) {
	auto it = begin;
	auto lastHrInVec = heartRateVecRaw.size() ? heartRateVecRaw.size() - 1 : 0;

	for (; it != sensorDataSet.end(); ++it) {
		auto sd = *it;
		if (beatDetector.addSample(sd.getMs(), sd.getIrLed() * -1)) {
			if (!beatSet.empty()) {
				heartRateVecRaw.push_back(HeartRate(
					(*beatSet.rbegin()),	// begin
					sd.getMs()				// end
				));
			}
			beatSet.insert(sd.getMs());
		}
	}

	// Compute quantile mean
	for (int i = lastHrInVec + 1; i < heartRateVecRaw.size(); ++i) {
		auto quantileBegin = heartRateVecRaw.begin();
		if((i - (int)quantileMeanN + 1) >= 0)
			std::advance(quantileBegin, i - quantileMeanN + 1);
		auto quantileEnd = heartRateVecRaw.begin();
		std::advance(quantileEnd, i + 1);

		heartRateVec.emplace_back(
			heartRateVecRaw.at(i).getBeginMs(),
			heartRateVecRaw.at(i).getEndMs(),
			quantileMean<HeartRate>(quantileBegin, quantileEnd,
				[](const HeartRate & v)->double { return v.getHR(); }
			)
		);
	}
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

std::vector<HeartRate>::iterator Data::getHeartRateBegin(qint64 laterThanMs) {
	return std::find_if(heartRateVec.begin(), heartRateVec.end(),
		[laterThanMs](const HeartRate & hr)->bool {
		return hr.getBeginMs() > laterThanMs;
	});
}

std::pair<double, double> Data::heartRateMinMax(
	const std::vector<HeartRate>::iterator & begin, 
	const std::vector<HeartRate>::iterator & end
) {
	auto minMax = std::minmax_element(begin, end, 
		[](const HeartRate & l, const HeartRate & r)->bool {
			return l.getHR() < r.getHR();
	});
	return std::pair<double, double>(
			(*(minMax.first)).getHR(),
			(*(minMax.second)).getHR()
		);
}

double Data::msToCustomPlotMs(qint64 ms) {
	return ms / 1000.0;
}

qint64 Data::customPlotMsToMs(double cMs) {
	return cMs * 1000 + 0.5;
}

std::string Data::timestampStringFromMsSinceEpoch(qint64 ms) {
	return QDateTime::fromMSecsSinceEpoch(ms)
		.toString("yyyy-MM-dd hh:mm:ss.zzz")
		.toStdString();
}
