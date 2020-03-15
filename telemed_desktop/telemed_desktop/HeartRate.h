#pragma once
#include <QString>
#include <QDateTime>

class HeartRate {
	qint64 begin = 0;
	qint64 end = 0;
	double hr = 0.0;
public:
	HeartRate(qint64 begin_, qint64 end_) :
		begin(begin_), end(end_) {
		hr = (60000.0 / (end_ - begin_));
	}
	HeartRate() {}
	bool operator<(const HeartRate & oth) const {
		return this->begin < oth.begin;
	}
	qint64 getBeginMs() const {
		return begin;
	}
	qint64 getEndMs() const {
		return end;
	}
	double getHR() const {
		return hr;
	}
	QString getBeginTimeStr() const {
		return dateStringFromMsSinceEpoch(begin);
	}
	QString getEndTimeStr() const {
		return dateStringFromMsSinceEpoch(end);
	}
	QString getHRStr() const {
		return QString::number(hr);
	}
	static QString dateStringFromMsSinceEpoch(qint64 ms) {
		return QDateTime::fromMSecsSinceEpoch(ms).toString("hh:mm:ss.zzz");
	}
};