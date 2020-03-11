#include "Data.h"

#include <nlohmann/json.h>
#include <QDateTime>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <iir/Butterworth.h>
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
	manager = new QNetworkAccessManager(this);

	connect(manager, &QNetworkAccessManager::finished, this, &Data::networkResponse);
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
	
}

Data::~Data()
{
}

void Data::timerTimeout() {
	dataSaved = false;

	QNetworkRequest request;
	request.setUrl(QUrl("http://192.168.0.100"));
	manager->get(request);
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
	/*
	QFile file(filepath);
	if (file.open(QIODevice::WriteOnly)) {
		QTextStream ts("");

		ts << "Stempel czasowy;Val\n";
		QVector<QString> rows(xData.size());
		
		std::transform(
			xData.begin(), 
			xData.end(), 
			yData.begin(), 
			rows.begin(), 
			[](const QDateTime & time, double val)->QString {
				return (time.toString("yyyy-MM-dd hh:mm:ss.zzz") + ";" + QString::number(val));
		});

		dataSaved = true;
	}
	else {
		dataSaved = false;
	}
	file.close();
	*/
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
	return ToCustomPlotMs()(xBeatData.back());
}

std::pair<double, double> Data::getMinMaxForLast(double secs) {
	qint64 beg = xMainData.back() - FromCustomPlotMs()(secs);
	auto itMain = std::find_if(xMainData.begin(), xMainData.end(),
		[beg](qint64 t)->bool {
			return t > beg;
	});
	beg = xBeatData.back() - FromCustomPlotMs()(secs);
	auto itBeat = std::find_if(xBeatData.begin(), xBeatData.end(),
		[beg](qint64 t)->bool {
			return t > beg;
	});
	auto mainBegId = std::distance(xMainData.begin(), itMain);
	auto beatBegId = std::distance(xBeatData.begin(), itBeat);

	auto minMaxMain = std::minmax_element(yMainData.begin() + mainBegId, yMainData.end());
	auto minMaxBeat = std::minmax_element(yBeatData.begin() + beatBegId, yBeatData.end());

	return std::pair<double, double>(
		std::min(*minMaxBeat.first, *minMaxMain.first),
		std::max(*minMaxBeat.second, *minMaxMain.second)
	);
}

int Data::getRangeSize(qint64 begin, const QVector<qint64> & vec) {
	auto beg = std::find(vec.begin(), vec.end(), begin);
	return std::distance(beg, vec.end());
}


void Data::networkResponse(QNetworkReply * reply) {
	if (reply->error()) {
		qDebug() << reply->errorString();
		return;
	}

	QString answer = reply->readAll();
	processNewData(std::move(answer));
}


void Data::processNewData(QString && data_) {
	nlohmann::json data = nlohmann::json::parse(data_.toStdString());

	// set begin time of measures
	if (begMs == 0) {
		auto max = std::max_element(data.begin(), data.end(), 
			[](const nlohmann::json::value_type & l, const nlohmann::json::value_type & r)->bool {
				return l["T"].get<unsigned long long>() < r["T"].get<unsigned long long>();
		});
		begMs = QDateTime::currentDateTime().toMSecsSinceEpoch() - (*max)["T"].get<unsigned long long>();
	}
	
	std::vector<std::pair<qint64, int>> vals(data.size());
	// map json to vector, normalize time and filter signal
	std::transform( data.begin(), data.end(), vals.begin(), 
		[this](const nlohmann::json::value_type & row)->std::pair<qint64, int> {
			return std::pair<qint64, int>(
				(row["T"].get<unsigned long long>() + this->begMs),
				this->filter.filter(row["V"].get<int>())
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
	//xBeatData.push_back(xMainData.back());
	//yBeatData.push_back(yMainData.back());

	emit receivedNewData();
}
