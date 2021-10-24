#include "ConnectionContext.h"

void libefa::ConnectionContext::startTimekeeper() {
	DEBUG("Starting timekeeper\n");
	start = FabricUtil::getTimeMicroSeconds();
}

void libefa::ConnectionContext::stopTimekeeper() {
    end = FabricUtil::getTimeMicroSeconds();
	DEBUG("Stopped timekeeper\n");
}