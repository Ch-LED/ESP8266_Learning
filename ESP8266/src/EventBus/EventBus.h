
# pragma once

# include <Arduino.h>
# include <map>
# include <vector>
# include <functional>

enum EventType {
    NETWORK_CONNECTED,
    NETWORK_DISCONNECTED,

    COMMAND_RECEIVED,
    COMMAND_PROCESSED,

    LED_STATE_CHANGED
};

struct Event {
    EventType type;
    void* data;
    size_t size;
};

class EventBus {
public:
    static void subscribe(EventType eventName, std::function<bool(Event)> callback) {
    subscribers[eventName].push_back(callback);
    }

    static bool publish(EventType eventName, void* data = nullptr, size_t size = 0) {
        Event event { eventName, data, size};

        if (subscribers.find(eventName) == subscribers.end()) {
            Serial.printf("[EventBus] No subscribers for event: %d\n", eventName);
            return false; // No subscribers for this event
        }

        for (auto& callback : subscribers[eventName]) {
            if (!callback(event)) {
                Serial.printf("[EventBus] Event %d handling stopped by a subscriber.\n", eventName);
                return false; // Stop publishing if any subscriber returns false
            }
        }
        Serial.printf("[EventBus] Event %d published to %d subscribers.\n", eventName, subscribers[eventName].size());
        return true;
    }

private:
    static std::map<EventType, std::vector<std::function<bool(Event)>>> subscribers;
};