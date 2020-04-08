#pragma once
#include <QString>
#include <QDateTime>

/**
 * Klasa reprezentująca puls. 
 */
class HeartRate {
	qint64 begin = 0;
	qint64 end = 0;
	double hr = 0.0;
public:
	/**
	 * Konstruktor inicjalizujący.
	 * @param begin_ Początek okresu pulsu w milisekundach.
	 * @param end_ Koniec okresu pulsu w milisekundach.
	 * @param hr_ Wartość pulsu.
	 */
	HeartRate(qint64 begin_, qint64 end_, double hr_) :
		begin(begin_), end(end_), hr(hr_) 
	{}

	/**
	 * Konstruktor inicjalizujący i wyznaczający puls.
	 * Puls wyznaczany jest na podstawie wzoru: 
	 *	Heart Rate = 60000/(end_ - begin_)
	 * @param begin_ Początek okresu pulsu w milisekundach.
	 * @param end_ Koniec okresu pulsu w milisekundach.
	 */
	HeartRate(qint64 begin_, qint64 end_) :
		begin(begin_), end(end_) {
		hr = (60000.0 / (end_ - begin_));
	}

	/**
	 * Domyślny konstruktor.
	 */
	HeartRate() {}

	/**
	 * Operator porównania, aby obiety klasy mogły być częścią kolekcji std::set.
	 */
	bool operator<(const HeartRate & oth) const {
		return this->begin < oth.begin;
	}

	/**
	 * Getter.
	 * @return Czas początku okresu pulsu w milisekundach.
	 */
	qint64 getBeginMs() const {
		return begin;
	}

	/**
	 * Getter.
	 * @return Czas początku okresu pulsu w milisekundach.
	 */
	qint64 getEndMs() const {
		return end;
	}

	/**
	 * Getter.
	 * Wartość pulsu w BPM.
	 */
	double getHR() const {
		return hr;
	}

	/**
	 * Getter.
	 * @return Czas początku okresu pulsu w formacie hh:mm:ss.zzz.
	 */
	QString getBeginTimeStr() const {
		return dateStringFromMsSinceEpoch(begin);
	}

	/**
	 * Getter.
	 * @return Czas początku okresu pulsu w formacie hh:mm:ss.zzz.
	 */
	QString getEndTimeStr() const {
		return dateStringFromMsSinceEpoch(end);
	}

	/**
	 * Getter.
	 * Wartość pulsu w BPM. 
	 */
	QString getHRStr() const {
		return QString::number(hr);
	}
	
	/**
	 * Konwerter czasu w milisekundach od początku epoki na string w formacie hh:mm:ss.zzz.
	 */
	static QString dateStringFromMsSinceEpoch(qint64 ms) {
		return QDateTime::fromMSecsSinceEpoch(ms).toString("hh:mm:ss.zzz");
	}
};