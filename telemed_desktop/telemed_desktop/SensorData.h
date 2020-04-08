#pragma once
#include <QString>

/**
 * Klasa reperzentująca pojedynczą próbkę danych odebrancyh z modułu WiFi.
 */
class SensorData {
	qint64 ms = 0;
	int ir = 0,
		red = 0;
public:
	/** 
	 * Konstruktor inicjalizujący wyłącznie stempel czasowy próbki.
	 * @param ms_ Stempel czasowy wykonania próbki.
	 */
	SensorData(qint64 ms_) :
		ms(ms_)
	{}

	/**
	 * Konstruktor inicjalizujący.
	 * @param ms_ Stempel czasowy wykonania próbki.
	 * @param ir_ Wartość odczytana z diody podczerwonej.
	 * @param red_ Wartość odczytana z diody czerwonej.
	 */
	SensorData(qint64 ms_, int ir_, int red_) :
		ms(ms_), ir(ir_), red(red_) {}

	/**
	 * Domyślny konstruktor.
	 */
	SensorData() {}

	/**
	 * Operator porównania, aby obiety klasy mogły być częścią kolekcji std::set.
	 */
	bool operator<(const SensorData & oth) const {
		return this->ms < oth.ms;
	}

	/**
	 * Getter.
	 * @return zwraca stempel czasowy próbki w milisekundach.
	 */
	qint64 getMs() const {
		return ms;
	}

	/**
	 * Getter.
	 * Zwraca  zwraca stempel czasowy próbki w formacie custo plot.
	 */
	double toCustomPlotMs() const {
		return ms / 1000.0;
	}

	/**
	 * Getter.
	 * @return Zwraca wartość odczytaną z pomiaru za pomocą diody podczerwonej.
	 */
	int getIrLed() const {
		return ir;
	}

	/**
	 * Getter.
	 * @return Zwraca wartość odczytaną z pomiaru za pomocą diody czerwonej.
	 */
	int getRedLed() const {
		return red;
	}
};