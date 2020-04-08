#pragma once

#include <QObject>
#include <QUrl>

class QNetworkReply;
class QNetworkAccessManager;

/**
 * Klasa odpowiedzialna za komunikację z modułem WiFi ESP8266-12E.
 */
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
	/**
	 * Wartości prądy możliwe do ustawienia. 
	 */
	enum LEDCurrent {
		MAX30100_LED_CURR_0MA = 0x00,		/**< 0 [mA] */
		MAX30100_LED_CURR_4_4MA = 0x01,		/**< 4.4 [mA] */
		MAX30100_LED_CURR_7_6MA = 0x02,		/**< 7.6 [mA] */
		MAX30100_LED_CURR_11MA = 0x03,		/**< 11 [mA] */
		MAX30100_LED_CURR_14_2MA = 0x04,	/**< 14.2 [mA] */
		MAX30100_LED_CURR_17_4MA = 0x05,	/**< 17.4 [mA] */
		MAX30100_LED_CURR_20_8MA = 0x06,	/**< 20.8 [mA] */
		MAX30100_LED_CURR_24MA = 0x07,		/**< 24 [mA] */
		MAX30100_LED_CURR_27_1MA = 0x08,	/**< 27.1 [mA] */
		MAX30100_LED_CURR_30_6MA = 0x09,	/**< 30.6 [mA] */
		MAX30100_LED_CURR_33_8MA = 0x0a,	/**< 33.8 [mA] */
		MAX30100_LED_CURR_37MA = 0x0b,		/**< 37 [mA] */
		MAX30100_LED_CURR_40_2MA = 0x0c,	/**< 40.2 [mA] */
		MAX30100_LED_CURR_43_6MA = 0x0d,	/**< 43.6 [mA] */
		MAX30100_LED_CURR_46_8MA = 0x0e,	/**< 46.8 [mA] */
		MAX30100_LED_CURR_50MA = 0x0f		/**< 50 [mA] */
	};

	/**
	 * Domyślny konstruktor.
	 * @param parent Przodek obiektu.
	 */
	DeviceApi(QObject *parent);

	/**
	 * Domyślny destruktor.
	 */
	~DeviceApi() {};

signals:
	/**
	 * Sygnał emitowany po otrzymaniu nowych danych.
	 * @param json Dane w formacie JSON.
	 */
	void newMeasuresData(const QString & json);

public slots:
	/**
	 *  Metoda służąca do wysłania żadania odczytu nowych danych typu GET na adres
	 *  <pre>http://192.168.4.1/</pre>
	 */
	void readNewMeasures();

	/**
	 * Metoda służąca do zmiany adresu ip modułu WiFi.
	 * W trybie Access Point używanie jej jest niezalecane.
	 */
	void setDeviceIp(const QString & ip);

	/**
	 * Metoda służąca do wysłania żadania zmiany prądu diody podczerwonej typu GET na adres
	 * @param I wartość prądu.
	 * <pre>http://192.168.4.1/set_led_current?ir=I</pre>
	 */
	void setIrLedCurrent(unsigned int I);

	/**
	 * Metoda służąca do wysłania żadania zmiany prądu diody czerwonej typu GET na adres
	 * @param I wartość prądu.
	 * <pre>http://192.168.4.1/set_led_current?red=I</pre>
	 */
	void setRedLedCurrent(unsigned int I);

private slots:
	void networkResponse(QNetworkReply * reply);
};
