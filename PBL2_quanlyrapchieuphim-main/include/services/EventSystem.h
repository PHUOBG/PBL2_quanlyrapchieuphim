#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>
#include <algorithm>
using namespace std;

class AppEvent {
public:
    virtual ~AppEvent() = default;
    virtual string getType() const = 0;
};

class UserRegisteredEvent : public AppEvent {
public:
    string email;
    string fullName;
    
    UserRegisteredEvent(const string& email, const string& fullName);
    string getType() const override;
};

class VoucherCreatedEvent : public AppEvent {
public:
    string code;
    bool autoProvision;
    int defaultDays;
    int defaultQuantity;
    
        VoucherCreatedEvent(const string& code, bool autoProvision = false,
                                                int defaultDays = 30, int defaultQuantity = 1);
        string getType() const override;
};

class IAppEventObserver {
public:
    virtual ~IAppEventObserver() = default;
    virtual void onAppEvent(const AppEvent& event) = 0;
    virtual vector<string> getSubscribedEvents() const = 0;
};

class AppEventSystem {
public:
    static AppEventSystem& getInstance();

    AppEventSystem(const AppEventSystem&) = delete;
    AppEventSystem& operator=(const AppEventSystem&) = delete;
    void subscribe(shared_ptr<IAppEventObserver> observer);
    void unsubscribe(shared_ptr<IAppEventObserver> observer);
    void publish(const AppEvent& event);
    
private:
    AppEventSystem() = default;
    
    unordered_map<string, vector<weak_ptr<IAppEventObserver>>> observers;
};
