
# include "EventBus/EventBus.h"

std::map<EventType, std::vector<std::function<bool(Event)>>> EventBus::subscribers;