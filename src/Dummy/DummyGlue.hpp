#ifndef _DUMMYGLUE_HPP_
#define _DUMMYGLUE_HPP_

#include "SglEvents.hpp"

void handleMemEvent(const Event<DataAccess>* ev);
void handleCompEvent(const Event<DataCompute>* ev);

#endif
