#pragma once

#include <QObject>
#include <set>
#include <vector>
#include <functional>
#include <iir/Butterworth.h>
#include "MAX30100_BeatDetector.h"
#include "HeartRate.h"
#include "SensorData.h"

auto constexpr TIMER_INTERVAL = 1000;	/**< Okres wysyłanie żądań typu GET w milisekundach. */
auto constexpr FILTER_ORDER = 2;		/**< Rząd filtru. */

auto constexpr SAMPLING_RATE = 100.0;	/**< Częstotliwość próbkowania czujnika w Hz. */
auto constexpr LOW_CUT_FREQ = 1.4;		/**< Dolna częstotliwość odcięcia filtru w Hz. */// Hz
auto constexpr HIGH_CUT_FREQ = 6;		/**< Górna częstotliwość odcięcia filtru w Hz. */// Hz

class QTimer;

/**
 * Klasa odpowiedzialna za przetwarzanie danych.
 */
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
	bool irDataEnabled = false;
	bool redDataEnabled = false;
	bool hrDataEnabled = false;

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
	static std::string timestampStringFromMsSinceEpoch(qint64 ms);

public:
	/**
	 * Domyślny konstruktor.
	 * @param parent Przodek obiektu.
	 */
	Data(QObject *parent);

	/**
	 * Domyślny destruktor.
	 */
	~Data() {};

	/**
	 * Metoda startuje timer.
	 */
	void start();

	/**
	 * Metoda zatrzymuje timer.
	 */
	void stop();

	/**
	 * Metoda czyści zgormadzone dane.
	 */
	void clear();

	/**
	 * Metoda zapisuje dane do pilku .xlsx.
	 * @param filepath Ścieżka do pliku.
	 * @see https://github.com/troldal/OpenXLSX
	 */
	void saveAs(const QString & filepath);

	/**
	 * Aktywuje serię danych związaną z diodą podczerwoną.
	 */
	void setIrLedEnabled(bool enabled);

	/**
	 * Aktywuje serię danych związaną z diodą czerwoną.
	 */
	void setRedLedEnabled(bool enabled);

	/**
	 * Aktywuje serią danych związaną z pulsem.
	 */
	void setHearRateEnabled(bool enabled);

	/**
	 * Getter
	 * @return Nazwa serii danych diody IR
	 */
	QString getYIrSensorDataName() const;
	
	/**
	 * Getter
	 * @return Nazwa serii danych diody RED
	 */
	QString getYRedSensorDataName() const;

	/**
	 * Getter
	 * @param dataLaterThan Prametr określający punkt w czasie 
	 *			(milisekundy od początku epoki) od którego mają zostać zwrócone wartości.
	 * @return Dane osi pozionej (X), czyli millisekundy zapisane jako double w formacie custom plot.
	 */
	QVector<double> getXSensorData(double dataLaterThan = -1.0);

	/**
	 * Getter
	 * @param dataLaterThan Prametr określający punkt w czasie
	 *			(milisekundy od początku epoki) od którego mają zostać zwrócone wartości.
	 * @return Dane pochodzące z diody podczerwonej - osi pionowej (Y), 
	 *			czyli millisekundy zapisane jako double w formacie custom plot.
	 */
	QVector<double> getYIrSensorData(double dataLaterThan = -1.0);

	/**
	 * Getter
	 * @param dataLaterThan Prametr określający punkt w czasie
	 *			(milisekundy od początku epoki) od którego mają zostać zwrócone wartości.
	 * @return Dane pochodzące z diody czerwoniej - osi pionowej (Y),
	 *			czyli millisekundy zapisane jako double w formacie custom plot.
	 */
	QVector<double> getYRedSensorData(double dataLaterThan = -1.0);

	/**
	 * Getter.
	 * @return Stempel czasowy ostatniej odczytanej danej z sensora, 
	 *		wyrażony w milisekundach w formacie custom plot.
	 */
	double getLastSensorDataCustomPlotMs();

	/**
	 * Getter.
	 * Wyznacza minimalną i maksymalną wartość z zakresu od ostatniej danej minus rangeSizeInSeconds do ostatniej danej.
	 * @param rangeSizeInSeconds zakres wyświetlanych danych.
	 * @return Minimalną i maksymalną wartość spośród serii:
	 *	- wartości diody podczerwonej,
	 *	- wartości diody czerwonej,
	 *	- wartości pulsu.
	 */
	std::pair<double, double> getDataMinMax(int rangeSizeInSeconds);

	/**
	 * Getter.
	 * @return Nazwę serii danych związanej z wykrytymi uderzeniami serca.
	 */
	QString getBeatDataName() const;

	/**
	 * Getter.
	 * @return Stemple czasowe uderzeń serca, milisekund w formacie custom plot.
	 */
	QVector<double> getXBeatData();

	/**
	 * @param rangeSizeInSeconds zakres wyświetlanych danych.
	 * @return Wartości Y odpowiadjące maksymalnej wartości zwróconej przez metodę getDataMinMax.
	 * @see Data::getDataMinMax
	 */
	QVector<double> getYBeatData(int rangeSizeInSeconds);

	/**
	 * Setter.
	 * @param n Liczba próbek do liczenia średniej.
	 */
	void setHeartRateQuantileN(unsigned int n);

	/**
	 * Getter.
	 * @return Nazwa serii pulsu.
	 */
	QString getHeartRateDataName() const;

	/**
	 * Getter.
	 * @param dataLaterThan Prametr określający punkt w czasie
     *			(milisekundy od początku epoki) od którego mają zostać zwrócone wartości.
	 * @return Wektor obiektów pulsu, nieuśrednione wartości.
	 */
	QVector<HeartRate> getHeartRate(qint64 laterThan = -1);

	/**
	 * Getter.
	 * @param dataLaterThan Prametr określający punkt w czasie
	 *			(milisekundy od początku epoki) od którego mają zostać zwrócone wartości.
	 * @return Wektor obiektów pulsu, uśrednione wartości.
	 */
	QVector<HeartRate> getQuantileMeanHeartRate(qint64 laterThan = -1);

	/**
	 * Getter.
	 * @return Stemple czasowe pulsu wyrażone w milisekudach sformatowanych zgodnie z formatem custom plot.
	 */
	QVector<double> getXHRData() const;

	/**
	 * Getter.
	 * @return Wartości pulsu wyrażone w uderzeniach serca na minutę [BPM].
	 */
	QVector<double> getYHRData() const;

	/**
	 * Konterter milisekund na format custom plot.
	 * Np. ms = 1200 [ms], zwraca ms/1000 = 1.2
	 * @param ms milisekundy
	 * @return milisekundy zgodne z formatem custom plot
	 */
	static double msToCustomPlotMs(qint64 ms);

	/**
	 * Konweter milisekund zapisanych w formacie custom plot na całkowitoliczbowe.
	 * @param cMs milisekundy w formacie custom plot.
	 * @return milisekundy.
	 */
	static qint64 customPlotMsToMs(double cMs);

	/**
	 * Metoda sprawdzająca flagę czy dane zostały zapisane.
	 * @return true jeżeli dane są zapisane do pliku.
	 * @return false jeżeli nie są zapisane.
	 */
	bool isDataSaved() { return dataSaved; };

protected slots:
	/**
	 * Motoda wywoływana po przepełnieniu timera, wysyła żadanie typu GET do modułu WiFi. 
	 * @see DeviceApi::readNewMeasures
	 */
	void timerTimeout();

	/**
	 * Metoda wywoływana po odebraniu nowej paczki danych.
	 * Średnio raz na sekundę. Ilość danych to 130 (cały bufor w module ESP8266-12E).
	 * Część danych jest pomijana ponieważ w ciągu  1 sekundy gromadzonych jest 100 pomiarów.
	 * Dzięki temu zabiegowi zapewniona jest ciągłość danych.
	 * Następnie dane podawane są filtracji za pomocą filtru pasmowoprzepustowgo IIR Butterworha drugiego rzędu.
	 * Dolna częstotliwość odcięcia to 1.4Hz, a górna 6Hz. 
	 * Przefiltrowane próbki pochodzące z diody podczerwonej trafiają do datektora uderzeń serca.
	 * @param data_ Dane odebrane z modułu WiFi.
	 * @see Data::detectHeartRate
	 * @see https://github.com/berndporr/iir1
	 */
	void processNewData(const QString & data_);

	/**
	 * Metoda służąca do detekcji pulsu.
	 * Na początek próbki trafiają do detektora uderzeń serca.
	 * Jeżeli wykryte zostanie uderzenie to następuje detekcja pulsu.
	 * @param begin Iterator początku nowych danych w zbiorze.
	 * @see https://github.com/oxullo/Arduino-MAX30100
	 */
	void detectHeartRate(std::set<SensorData>::iterator begin);

signals:
	/**
	 * Sygnał emitowany w momencie zakończenia analizy nowych danych.
	 */
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
