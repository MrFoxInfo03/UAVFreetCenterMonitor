#ifndef NATS_CONN_H
#define NATS_CONN_H

#include <iostream>
#include <nats.h>
#include <string>

class NatsConnection
{
private:
    natsConnection* conn = nullptr;

public:
    NatsConnection() = default;

    ~NatsConnection() {
        if(conn != nullptr) {
            natsConnection_Close(conn);
            natsConnection_Destroy(conn);
            std::cout << "NATS connection closed.\n";
        }
    }

    bool Connect(const std::string& url = "nats://127.0.0.1:4222")
    {
        natsStatus status = natsConnection_ConnectTo(&conn, url.c_str());

        if(status != NATS_OK) {
            std::cerr << "NATS connection error: " << natsStatus_GetText(status) << '\n';
            conn = nullptr; 
            return false;
        }
    }

    bool Publish(const std::string& subject, const std::string& data)
    {
        if(conn == nullptr) {
            std::cerr << "NATS not connected.\n";
            return false;
        }

        natsStatus status = natsConnection_PublishString(conn, subject.c_str(), data.c_str());

        if(status != NATS_OK) {
            std::cerr << "NATS publish error: " << natsStatus_GetText(status) << '\n';
            return false;
        }

        return true;
    }
};

#endif