#pragma once
#include <QObject>
#include <map>
#include <typeinfo>

/**
 * Klasa tworząca instancje obietów dziedziczących po QObject, możliwy dostęp w dowolnym fragemncie kodu.
 */
class ObjectFactory
{
	// TODO: Smart pointers
	static std::map<size_t, QObject *> objects;

public:
	/**
	 * Metoda przyjmująca surowy wskaźnik na instancję nowego obiektu typu T.
	 */
	template<class T>
	static void createInstance(T * obj);

	/**
	 * Getter.
	 * @return Zwraca instancję obiektu typu T.
	 */
	template<class T>
	static T * getInstance();

	/**
	 * Metoda służąca do sprawdzenia czy istnieje instanacja obiektu typu T.
	 * @return true Jeżeli istanie.
	 * @return false Jeżeli nie została dotychczas stowrzona instancja obiektu typu T.
	 */
	template<class T>
	static bool hasInstance();

	/**
	 * Metoda usuwająca wszystkie instancje obiektów w obrębie fabryki.
	 */
	static void deleteFactory();
};

template<class T>
inline void ObjectFactory::createInstance(T * obj)
{
	const size_t hash = typeid(T).hash_code();
	if (hasInstance<T>())
		delete objects[hash];
	objects[hash] = qobject_cast<QObject *>(obj);
}

template<class T>
inline T * ObjectFactory::getInstance()
{
	const size_t hash = typeid(T).hash_code();
	QObject * obj = objects.at(hash);
	T * instance = qobject_cast<T *>(obj);
	Q_ASSERT(instance != nullptr);

	return instance;
}

template<class T>
inline bool ObjectFactory::hasInstance() {
	const size_t hash = typeid(T).hash_code();
	return (objects.find(hash) != objects.end());
}
