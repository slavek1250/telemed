#pragma once
#include <QString>

class SensorData {
	qint64 ms = 0;
	int ir = 0,
		red = 0;
public:
	SensorData(qint64 ms_) :
		ms(ms_)
	{}
	SensorData(qint64 ms_, int ir_, int red_) :
		ms(ms_), ir(ir_), red(red_) {}
	SensorData() {}
	bool operator<(const SensorData & oth) const {
		return this->ms < oth.ms;
	}
	double toCustomPlotMs() const {
		return ms / 1000.0;
	}
	static SensorData fromCustomPlotMs(double ms) {
		return SensorData(ms * 1000 + 0.5);
	}
	qint64 getMs() const {
		return ms;
	}
	int getIrLed() const {
		return ir;
	}
	int getRedLed() const {
		return red;
	}
};