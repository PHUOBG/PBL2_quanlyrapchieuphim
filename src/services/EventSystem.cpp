#include "services/EventSystem.h"

UserRegisteredEvent::UserRegisteredEvent(const std::string& email, const std::string& fullName)
    : email(email), fullName(fullName) {}

std::string UserRegisteredEvent::getType() const { return "UserRegistered"; }

VoucherCreatedEvent::VoucherCreatedEvent(const std::string& code, bool autoProvision,
                                         int defaultDays, int defaultQuantity)
    : code(code), autoProvision(autoProvision), defaultDays(defaultDays), defaultQuantity(defaultQuantity) {}

std::string VoucherCreatedEvent::getType() const { return "VoucherCreated"; }

AppEventSystem& AppEventSystem::getInstance() {
    static AppEventSystem instance;
    return instance;
}

void AppEventSystem::subscribe(std::shared_ptr<IAppEventObserver> observer) {
    for (const auto& eventType : observer->getSubscribedEvents()) {
        observers[eventType].push_back(observer);
    }
}

void AppEventSystem::unsubscribe(std::shared_ptr<IAppEventObserver> observer) {
    for (auto& [type, observerList] : observers) {
        observerList.erase(
            std::remove_if(observerList.begin(), observerList.end(),
                           [&observer](const std::weak_ptr<IAppEventObserver>& weak) {
                               auto locked = weak.lock();
                               return !locked || locked == observer;
                           }),
            observerList.end());
    }
}

void AppEventSystem::publish(const AppEvent& event) {
    auto it = observers.find(event.getType());
    if (it != observers.end()) {
        // Clean up expired weak pointers and notify observers
        std::vector<std::weak_ptr<IAppEventObserver>>& observerList = it->second;
        for (auto iter = observerList.begin(); iter != observerList.end();) {
            if (auto locked = iter->lock()) {
                locked->onAppEvent(event);
                ++iter;
            }
            else {
                iter = observerList.erase(iter);
            }
        }
    }
}
