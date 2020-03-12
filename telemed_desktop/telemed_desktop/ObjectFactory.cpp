#include "ObjectFactory.h"

std::map<size_t, QObject *> ObjectFactory::objects = std::map<size_t, QObject *>();

void ObjectFactory::deleteFactory() {
	for (auto & obj : objects) {
		if(obj.second != nullptr)
			delete obj.second;
	}
}
