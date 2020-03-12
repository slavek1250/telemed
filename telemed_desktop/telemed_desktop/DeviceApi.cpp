#include "DeviceApi.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>


DeviceApi::DeviceApi(QObject *parent)
	: QObject(parent)
{
	manager = new QNetworkAccessManager(this);
	connect(manager, &QNetworkAccessManager::finished, this, &DeviceApi::networkResponse);
}

void DeviceApi::setLedCurrent(const QString & ledName, unsigned int I) {
	QNetworkRequest request;
	request.setUrl(
		QUrl(tr("http://%1/set_led_current?%2=%3")
			.arg(devIp)
			.arg(ledName)
			.arg((LEDCurrent)I)
	));
	manager->get(request);
}

void DeviceApi::setDeviceIp(const QString & ip) {
	devIp = ip;
}

void DeviceApi::readNewMeasures() {
	responseSource = DATA;
	QNetworkRequest request;
	request.setUrl(QUrl(tr("http://%1").arg(devIp)));
	manager->get(request);
}

void DeviceApi::setIrLedCurrent(unsigned int I) {
	responseSource = IR;
	setLedCurrent("ir", I);
}

void DeviceApi::setRedLedCurrent(unsigned int I) {
	responseSource = RED;
	setLedCurrent("red", I);
}

void DeviceApi::networkResponse(QNetworkReply * reply) {
	if (reply->error()) {
		qDebug() << reply->errorString();
		return;
	}
	if (responseSource == DATA) {
		QString answer = reply->readAll();
		emit newMeasuresData(std::move(answer));
	}
}