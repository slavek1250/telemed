#pragma once

#include <QObject>
#include <QUrl>

class QNetworkReply;
class QNetworkAccessManager;

class DeviceApi : public QObject
{
	Q_OBJECT
private:
	QNetworkAccessManager * manager;
	QString devIp;

	enum ResponseSource {
		DATA,
		IR,
		RED
	} responseSource = DATA;

	void setLedCurrent(const QString & ledName, unsigned int I);

public:
	enum LEDCurrent {
		MAX30100_LED_CURR_0MA = 0x00,
		MAX30100_LED_CURR_4_4MA = 0x01,
		MAX30100_LED_CURR_7_6MA = 0x02,
		MAX30100_LED_CURR_11MA = 0x03,
		MAX30100_LED_CURR_14_2MA = 0x04,
		MAX30100_LED_CURR_17_4MA = 0x05,
		MAX30100_LED_CURR_20_8MA = 0x06,
		MAX30100_LED_CURR_24MA = 0x07,
		MAX30100_LED_CURR_27_1MA = 0x08,
		MAX30100_LED_CURR_30_6MA = 0x09,
		MAX30100_LED_CURR_33_8MA = 0x0a,
		MAX30100_LED_CURR_37MA = 0x0b,
		MAX30100_LED_CURR_40_2MA = 0x0c,
		MAX30100_LED_CURR_43_6MA = 0x0d,
		MAX30100_LED_CURR_46_8MA = 0x0e,
		MAX30100_LED_CURR_50MA = 0x0f
	};

	DeviceApi(QObject *parent);
	~DeviceApi() {};

signals:
	void newMeasuresData(const QString & json);

public slots:
	void readNewMeasures();
	void setDeviceIp(const QString & ip);
	void setIrLedCurrent(unsigned int I);
	void setRedLedCurrent(unsigned int I);

private slots:
	void networkResponse(QNetworkReply * reply);
};
