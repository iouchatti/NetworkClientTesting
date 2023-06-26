#include <iostream>
#include <fstream>
#include <array>
#include <chrono>
#include <vector>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <boost/asio.hpp>
#include <thread>
#include <boost/bind/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <nlohmann/json.hpp>

#include "client.h"
#include "logger.h"

using json = nlohmann::json;

inline std::string format_time_point(std::chrono::steady_clock::time_point tp) {
    auto time_now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(time_now - tp);
    auto time_t_diff = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - duration);
    std::tm tm = {};
    localtime_s(&tm, &time_t_diff);

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %X");
    return ss.str();
}

bool readConfigFile(const std::string& filename, json& config) {
    try {
        std::ifstream configFile(filename);
        if (!configFile.is_open()) {
            return false;
        }

        configFile >> config;
    }
    catch (const std::exception& e) {
        Logger::log("Exception readConfigFile: " + std::string(e.what()));
    }
    return true;
}

void createDefaultConfigFile(const std::string& filename, bool developmentMode) {
    try {
        json config;
        config["host"] = "127.0.0.1";
        config["port"] = 12345;
        config["testCases"] = {
            {
                {"name", "Manage absence of correspondent"},
                {"waitTime", 0},
                {"connectionTimeout", 30},
                {"clients",
                 {
                     {
                         {"clientName", "clientA"},
                         {"connectTime", 2},
                         {"disconnectTime", 30},
                         {"writeTime", 0},
                         {"message", ""},
                         {"password", "password123"}
                     }
                 }}
            },
            {
                {"name", "Manage basic communication"},
                {"waitTime", 1},
                {"connectionTimeout", 30},
                {"clients",
                 {
                     {
                         {"clientName", "clientA"},
                         {"connectTime", 2},
                         {"disconnectTime", 30},
                         {"writeTime", 5},
                         {"message", "Hello from clientA"},
                         {"password", "password123"}
                     },
                     {
                         {"clientName", "clientB"},
                         {"connectTime", 5},
                         {"disconnectTime", 30},
                         {"writeTime", 0},
                         {"message", ""},
                         {"password", "password456"}
                     }
                 }}
            },
            {
                {"name", "Manage bidirectional communication"},
                {"waitTime", 0},
                {"connectionTimeout", 30},
                {"clients",
                 {
                     {
                         {"clientName", "clientA"},
                         {"connectTime", 2},
                         {"disconnectTime", 30},
                         {"writeTime", 5},
                         {"message", "[CMD]ECHOREPLY snowpack"},
                         {"password", "password123"}
                     },
                     {
                         {"clientName", "clientB"},
                         {"connectTime", 5},
                         {"disconnectTime", 30},
                         {"writeTime", 0},
                         {"message", ""},
                         {"password", "password456"}
                     }
                 }}
            },
            {
                {"name", "Manage bidirectional communication but connect to correspondent based on dedicated secret – Same secrets"},
                {"waitTime", 0},
                {"connectionTimeout", 30},
                {"clients",
                 {
                     {
                         {"clientName", "clientA"},
                         {"connectTime", 2},
                         {"disconnectTime", 30},
                         {"writeTime", 5},
                         {"message", "[CMD]ECHOREPLY snowpack"},
                         {"password", "password123"}
                     },
                     {
                         {"clientName", "clientB"},
                         {"connectTime", 5},
                         {"disconnectTime", 30},
                         {"writeTime", 0},
                         {"message", ""},
                         {"password", "password123"}
                     }
                 }}
            },
            {
                {"name", "Manage bidirectional communication but connect to correspondent based on dedicated secret – Different secrets"},
                {"waitTime", 0},
                {"connectionTimeout", 30},
                {"clients",
                 {
                     {
                         {"clientName", "clientA"},
                         {"connectTime", 2},
                         {"disconnectTime", 30},
                         {"writeTime", 5},
                         {"message", "[CMD]ECHOREPLY snowpack"},
                         {"password", "password123"}
                     },
                     {
                         {"clientName", "clientB"},
                         {"connectTime", 5},
                         {"disconnectTime", 30},
                         {"writeTime", 0},
                         {"message", "Hi I have a different pass"},
                         {"password", "password456"}
                     }
                 }
            }
            }
        };

        std::ofstream configFile(filename);
        if (configFile.is_open()) {
            configFile << std::setw(4) << config << std::endl;
            configFile.close();
        }
        else {
            Logger::log("Failed to open config file for writing. Exiting.");
            return;
        }
    }
    catch (const std::exception& e) {
        Logger::log("Exception in createDefaultConfigFile: " + std::string(e.what()));
    }
}

void clearLogFile(const std::string& filename) {
    std::ofstream logFile(filename, std::ofstream::trunc);
    if (logFile.is_open()) {
        logFile.close();
        Logger::log("Log file cleared.");
    }
    else {
        Logger::log("Failed to clear log file.");
    }
}
void executeTestCaseManually(const json& testCase, const std::string& host, short port) {
    std::string testName = testCase["name"].get<std::string>();
    Logger::log("Test Case: " + testName);

    Logger::log("- P is waiting for incoming connection");

    boost::asio::io_context ioContext;
    std::thread ioContextThread([&]() {
        try {
            ioContext.run();
        }
        catch (const std::exception& e) {
            Logger::log("Exception in event loop: " + std::string(e.what()));
        }
        });

    std::shared_ptr<Client> clientA, clientB;

    // Function to handle user input for sending messages
    auto handleUserInput = [&](std::shared_ptr<Client> client) {
        std::string input;
        std::getline(std::cin, input);
        auto in = std::make_shared<std::string>(input);
        client->setMessage(in);
        client->doWrite();
    };

    // Create clientA
    json clientAConfig = testCase["clients"][0];
    std::string clientAName = clientAConfig["clientName"].get<std::string>();
    std::string clientAMessage = clientAConfig["message"].get<std::string>();
    int clientAConnectTime = clientAConfig["connectTime"].get<int>();
    int clientADisconnectTime = clientAConfig["disconnectTime"].get<int>();
    int clientAWriteTime = clientAConfig["writeTime"].get<int>();
    std::string clientAPassword = clientAConfig["password"].get<std::string>();
    auto msgA = std::make_shared<std::string>(clientAMessage);
    ioContext.post([&]() {
        try {
            Logger::log("Connecting client " + clientAName + "...");
            clientA = std::make_shared<Client>(ioContext, host, port, clientAName);
            clientA->connect();
            Logger::log("Client " + clientAName + " connected");

            ioContext.post([&]() {
                std::this_thread::sleep_for(std::chrono::seconds(clientAWriteTime));
                Logger::log("Writing message for client " + clientAName + "...");
                Logger::log("Client " + clientAName + " wrote message: " + clientAMessage);
                clientA->setMessage(msgA);
                clientA->doWrite();
                });
        }
        catch (const std::exception& e) {
            Logger::log("Exception creating client " + clientAName + ": " + std::string(e.what()));
        }
        });

    // Create clientB
    json clientBConfig = testCase["clients"][1];
    std::string clientBName = clientBConfig["clientName"].get<std::string>();
    std::string clientBMessage = clientBConfig["message"].get<std::string>();
    int clientBConnectTime = clientBConfig["connectTime"].get<int>();
    int clientBDisconnectTime = clientBConfig["disconnectTime"].get<int>();
    int clientBWriteTime = clientBConfig["writeTime"].get<int>();
    std::string clientBPassword = clientBConfig["password"].get<std::string>();

    auto msgB = std::make_shared<std::string>(clientBMessage);
    ioContext.post([&]() {
        try {
            Logger::log("Connecting client " + clientBName + "...");
            clientB = std::make_shared<Client>(ioContext, host, port, clientBName);
            clientB->connect();
            Logger::log("Client " + clientBName + " connected");

            ioContext.post([&]() {
                std::this_thread::sleep_for(std::chrono::seconds(clientBWriteTime));
                Logger::log("Writing message for client " + clientBName + "...");
                Logger::log("Client " + clientBName + " wrote message: " + clientBMessage);
                clientB->setMessage(msgB);
                clientB->doWrite();
                });
        }
        catch (const std::exception& e) {
            Logger::log("Exception creating client " + clientBName + ": " + std::string(e.what()));
        }
        });

    // Wait for user input and handle sending messages
    char choice;
    std::cin >> choice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (choice == 'm' || choice == 'M') {
        // Manual mode - handle user input for sending messages
        Logger::log("Manual mode: Enter messages to send.");
        Logger::log("Type 'exit' to quit.");

        std::thread inputThreadA([&]() {
            while (true) {
                handleUserInput(clientA);
                if (*clientA->getMessage() == "exit") {
                    break;
                }
            }
            });

        std::thread inputThreadB([&]() {
            while (true) {
                handleUserInput(clientB);
                if (*clientB->getMessage() == "exit") {
                    break;
                }
            }
            });

        inputThreadA.join();
        inputThreadB.join();
    }
    else {
        // Automatic mode - wait for the test case to complete
        std::this_thread::sleep_for(std::chrono::seconds(testCase["connectionTimeout"].get<int>()));
    }

    // Disconnect clients
    ioContext.post([&]() {
        if (clientA) {
            Logger::log("Disconnecting client " + clientAName + "...");
            clientA->disconnect();
            Logger::log("Client " + clientAName + " disconnected");
        }
        if (clientB) {
            Logger::log("Disconnecting client " + clientBName + "...");
            clientB->disconnect();
            Logger::log("Client " + clientBName + " disconnected");
        }
        });

    // Stop the IO context
    ioContext.stop();
    ioContextThread.join();

    Logger::log("All operations completed for test case: " + testName);
}

void executeTestCaseAutomatically(const json& testCase, const std::string& host, short port) {
    std::string testName = testCase["name"].get<std::string>();
    Logger::log("Test Case: " + testName);

    Logger::log("- P is waiting for incoming connection");

    boost::asio::io_context ioContext;
    std::thread ioContextThread([&]() {
        try {
            ioContext.run();
        }
        catch (const std::exception& e) {
            Logger::log("Exception in event loop: " + std::string(e.what()));
        }
        });

    std::vector<std::shared_ptr<Client>> clients;

    // A function to disconnect and remove a client from the list
    auto disconnectClient = [&](const std::shared_ptr<Client>& client) {
        client->disconnect();
        clients.erase(std::remove_if(clients.begin(), clients.end(), [&](const std::shared_ptr<Client>& c) {
            return c == client;
            }), clients.end());
    };

    // For each client...
    for (const auto& clientConfig : testCase["clients"]) {
        std::string clientName = clientConfig["clientName"].get<std::string>();
        std::string message = clientConfig["message"].get<std::string>();
        int connectTime = clientConfig["connectTime"].get<int>();
        int disconnectTime = clientConfig["disconnectTime"].get<int>();
        int writeTime = clientConfig["writeTime"].get<int>();
        std::string password = clientConfig["password"].get<std::string>();

        ioContext.post([&, clientName, password]() {
            try {
                Logger::log("Connecting client " + clientName + "...");
                auto client = std::make_shared<Client>(ioContext, host, port, clientName);
                clients.push_back(client);
                client->connect();
                Logger::log("Client " + clientName + " connected");

                ioContext.post([&, client, message]() {
                    std::this_thread::sleep_for(std::chrono::seconds(writeTime));
                    Logger::log("Writing message for client " + clientName + "...");
                    Logger::log("Client " + clientName + " wrote message: " + message);
                    client->setMessage(std::make_shared<std::string>(message));
                    client->doWrite();
                    });

                ioContext.post([&, client, disconnectTime]() {
                    std::this_thread::sleep_for(std::chrono::seconds(disconnectTime));
                    Logger::log("Disconnecting client " + clientName + "...");
                    disconnectClient(client);
                    Logger::log("Client " + clientName + " disconnected");
                    });
            }
            catch (const std::exception& e) {
                Logger::log("Exception creating client " + clientName + ": " + std::string(e.what()));
            }
            });
    }

    // Wait for the test case to complete
    std::this_thread::sleep_for(std::chrono::seconds(testCase["connectionTimeout"].get<int>()));

    // Disconnect remaining clients
    for (const auto& client : clients) {
        disconnectClient(client);
    }

    // Stop the IO context
    ioContext.stop();
    ioContextThread.join();

    Logger::log("All operations completed for test case: " + testName);
}

int main() {
    const std::string logFile = "log.txt";
    // Clear the log file before starting the program
    clearLogFile(logFile);

    try {
        const std::string configFile = "config.json";
        json config;
        bool developmentMode = false;  // Set developmentMode to true to rewrite the config file

        if (!readConfigFile(configFile, config) || developmentMode) {
            // If the configuration file is not found or in development mode, create a default config file
            Logger::log("Configuration file not found or in development mode. Creating default config file.");
            createDefaultConfigFile(configFile, developmentMode);
            if (!readConfigFile(configFile, config)) {
                Logger::log("Failed to read default config file. Exiting.");
                return 1;
            }
        }

        // Extract host and port from configurations
        std::string host = config["host"].get<std::string>();
        short port = config["port"].get<short>();

        // Test Cases
        for (const auto& testCase : config["testCases"]) {
            std::string testCaseName = testCase["name"].get<std::string>();
            Logger::log("Executing test case: " + testCaseName);

            std::cout << "Press Enter to execute test case automatically or 'm' to execute it manually: ";
            std::string choice;
            std::getline(std::cin, choice);

            if (choice == "m") {
                executeTestCaseManually(testCase, host, port);
            }
            else {
                executeTestCaseAutomatically(testCase, host, port);
            }
        }
    }
    catch (const std::exception& e) {
        Logger::log("Exception in main: " + std::string(e.what()));
        return 1;
    }

    return 0;
}
