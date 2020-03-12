#pragma once
#include <QObject>
#include <map>
#include <typeinfo>

class ObjectFactory
{
	// TODO: Smart pointers
	static std::map<size_t, QObject *> objects;

public:
	template<class T>
	static void createInstance(T * obj);
	template<class T>
	static T * getInstance();
	template<class T>
	static bool hasInstance();
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
