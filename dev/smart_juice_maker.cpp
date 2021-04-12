/*
  using Mathieu Stefani's example, 07 février 2016
  Rares Cristea, 12.03.2021
  Smart Juice Maker Project
   
*/

#include <algorithm>

#include <pistache/net.h>
#include <pistache/http.h>
#include <pistache/peer.h>
#include <pistache/http_headers.h>
#include <pistache/cookie.h>
#include <pistache/router.h>
#include <pistache/endpoint.h>
#include <pistache/common.h>
#include <bits/stdc++.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <string>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>
#include <chrono>
#include <ctime>

#include <signal.h>

using namespace std;
using namespace Pistache;
using json = nlohmann::json;

// Declare all helper files
const std::string caloriesFile = "fruits_calories.txt";
const std::string vitaminsFile = "fruits_vitamins.txt";
const std::string juiceHistoryDB = "juices.txt";

// Helper class
class Helpers
{
public:
    const static std::string currentDateTime();
};

const std::string Helpers::currentDateTime()
{
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}
// General advice: pay atetntion to the namespaces that you use in various contexts. Could prevent headaches.
// This is just a helper function to preety-print the Cookies that one of the enpoints shall receive.
void printCookies(const Http::Request &req)
{
    auto cookies = req.cookies();
    std::cout << "Cookies: [" << std::endl;
    const std::string indent(4, ' ');
    for (const auto &c : cookies)
    {
        std::cout << indent << c.name << " = " << c.value << std::endl;
    }
    std::cout << "]" << std::endl;
}

class FruitCalories
{
public:
    std::string name;
    double calories;
};

class Fruit
{
public:
    std::string name;
    double quantity;
    std::vector<string> vitamins;
};

class Juice
{
public:
    static int id;
    double calories;
    double quantity;
    const string preparationDate = Helpers::currentDateTime();
    std::vector<string> vitamins;
    std::vector<string> fruits;
};

// TODO: get the number of juice form juices.txt
int Juice::id = 0;

// from_json overloading
void from_json(const json &json, Fruit &fruit)
{
    json.at("fruit").get_to(fruit.name);
    json.at("quantity").get_to(fruit.quantity);
}

void from_json(const json &json, FruitCalories &fruitCalories)
{
    json.at("fruit").get_to(fruitCalories.name);
    json.at("calories").get_to(fruitCalories.calories);
}

// to_json overloading
void to_json(json &j, const Juice &juice)
{
    j = json{
        {"id", juice.id},
        {"calories", juice.calories},
        {"quantity", juice.quantity},
        {"preparationDate", juice.preparationDate},
        {"fruits", juice.fruits}};
}

// Some generic namespace, with a simple function we could use to test the creation of the endpoints.
namespace Generic
{
    void handleReady(const Rest::Request &, Http::ResponseWriter response)
    {
        response.send(Http::Code::Ok, "1");
    }

    void makeJuice(const Rest::Request &request, Http::ResponseWriter response)
    {
        // from client
        auto clientJson = nlohmann::json::parse(request.body());
        auto clientFruits = clientJson.get<std::vector<Fruit>>();

        std::ifstream read(caloriesFile);
        json in = json::parse(read);
        auto fruitCalories = in.get<std::vector<FruitCalories>>();

        Juice newJuice;
        Juice::id++;
        newJuice.quantity = 0;
        newJuice.calories = 0;
        for (auto fruitClient : clientFruits)
        {
            for (auto fruitCal : fruitCalories)
            {
                if (fruitClient.name == fruitCal.name)
                {
                    newJuice.fruits.push_back(fruitClient.name);
                    newJuice.quantity += fruitClient.quantity;
                    newJuice.calories += (fruitCal.calories * fruitClient.quantity) / 100;
                }
            }
        }

        json juice(newJuice);

        response.send(Http::Code::Ok, juice.dump());
    }
}

// Definition of the SmartJuiceMakerEndpoint class
class SmartJuiceMakerEndpoint
{
public:
    explicit SmartJuiceMakerEndpoint(Address addr)
        : httpEndpoint(std::make_shared<Http::Endpoint>(addr))
    {
    }

    // Initialization of the server. Additional options can be provided here
    void init(size_t thr = 2)
    {
        auto opts = Http::Endpoint::options()
                        .threads(static_cast<int>(thr));
        httpEndpoint->init(opts);
        // Server routes are loaded up
        setupRoutes();
    }

    // Server is started threaded.
    void start()
    {
        httpEndpoint->setHandler(router.handler());
        httpEndpoint->serveThreaded();
    }

    // When signaled server shuts down
    void stop()
    {
        httpEndpoint->shutdown();
    }

private:
    void setupRoutes()
    {
        using namespace Rest;
        // Defining various endpoints
        // Generally say that when http://localhost:9080/ready is called, the handleReady function should be called.
        Routes::Get(router, "/ready", Routes::bind(&Generic::handleReady));
        Routes::Post(router, "/makeJuice", Routes::bind(&Generic::makeJuice));
        Routes::Get(router, "/auth", Routes::bind(&SmartJuiceMakerEndpoint::doAuth, this));
        Routes::Post(router, "/settings/:settingName/:value", Routes::bind(&SmartJuiceMakerEndpoint::setSetting, this));
        Routes::Get(router, "/settings/:settingName/", Routes::bind(&SmartJuiceMakerEndpoint::getSetting, this));
    }

    void doAuth(const Rest::Request &request, Http::ResponseWriter response)
    {
        // Function that prints cookies
        printCookies(request);
        // In the response object, it adds a cookie regarding the communications language.
        response.cookies()
            .add(Http::Cookie("lang", "en-US"));
        // Send the response
        response.send(Http::Code::Ok);
    }

    // Endpoint to configure one of the SmartJuiceMaker's settings.
    void setSetting(const Rest::Request &request, Http::ResponseWriter response)
    {
        // You don't know what the parameter content that you receive is, but you should
        // try to cast it to some data structure. Here, I cast the settingName to string.
        auto settingName = request.param(":settingName").as<std::string>();

        // This is a guard that prevents editing the same value by two concurent threads.
        Guard guard(smartJuiceMakerLock);

        string val = "";
        if (request.hasParam(":value"))
        {
            auto value = request.param(":value");
            val = value.as<string>();
        }

        // Setting the smart juice maker's setting to value
        int setResponse = sjm.set(settingName, val);

        // Sending some confirmation or error response.
        if (setResponse == 1)
        {
            response.send(Http::Code::Ok, settingName + " was set to " + val);
        }
        else
        {
            response.send(Http::Code::Not_Found, settingName + " was not found and or '" + val + "' was not a valid value ");
        }
    }

    // Setting to get the settings value of one of the configurations of the SmartJuiceMaker
    void getSetting(const Rest::Request &request, Http::ResponseWriter response)
    {
        auto settingName = request.param(":settingName").as<std::string>();

        Guard guard(smartJuiceMakerLock);

        string valueSetting = sjm.get(settingName);

        if (valueSetting != "")
        {

            // In this response I also add a couple of headers, describing the server that sent this response, and the way the content is formatted.
            using namespace Http;
            response.headers()
                .add<Header::Server>("pistache/0.1")
                .add<Header::ContentType>(MIME(Text, Plain));

            response.send(Http::Code::Ok, settingName + " is " + valueSetting);
        }
        else
        {
            response.send(Http::Code::Not_Found, settingName + " was not found");
        }
    }

    // Defining the class of the SmartJuiceMaker. It should model the entire configuration of the SmartJuiceMaker
    class SmartJuiceMaker
    {
    public:
        explicit SmartJuiceMaker() {}

        // Setting the value for one of the settings. Hardcoded for the defrosting option
        int set(std::string name, std::string value)
        {
            if (name == "defrost")
            {
                defrost.name = name;
                if (value == "true")
                {
                    defrost.value = true;
                    return 1;
                }
                if (value == "false")
                {
                    defrost.value = false;
                    return 1;
                }
            }
            return 0;
        }

        // Getter
        string get(std::string name)
        {
            if (name == "defrost")
            {
                return std::to_string(defrost.value);
            }
            else
            {
                return "";
            }
        }

    private:
        // Defining and instantiating settings.
        struct boolSetting
        {
            std::string name;
            bool value;
        } defrost;
    };

    // Create the lock which prevents concurrent editing of the same variable
    using Lock = std::mutex;
    using Guard = std::lock_guard<Lock>;
    Lock smartJuiceMakerLock;

    // Instance of the smart juice maker model
    SmartJuiceMaker sjm;

    // Defining the httpEndpoint and a router.
    std::shared_ptr<Http::Endpoint> httpEndpoint;
    Rest::Router router;
};

int main(int argc, char *argv[])
{

    // This code is needed for gracefull shutdown of the server when no longer needed.
    sigset_t signals;
    if (sigemptyset(&signals) != 0 || sigaddset(&signals, SIGTERM) != 0 || sigaddset(&signals, SIGINT) != 0 || sigaddset(&signals, SIGHUP) != 0 || pthread_sigmask(SIG_BLOCK, &signals, nullptr) != 0)
    {
        perror("install signal handler failed");
        return 1;
    }

    // Set a port on which your server to communicate
    Port port(9080);

    // Number of threads used by the server
    int thr = 2;

    if (argc >= 2)
    {
        port = static_cast<uint16_t>(std::stol(argv[1]));

        if (argc == 3)
            thr = std::stoi(argv[2]);
    }

    Address addr(Ipv4::any(), port);

    cout << "Cores = " << hardware_concurrency() << endl;
    cout << "Using " << thr << " threads" << endl;

    // Instance of the class that defines what the server can do.
    SmartJuiceMakerEndpoint stats(addr);

    // Initialize and start the server
    stats.init(thr);
    stats.start();

    // Code that waits for the shutdown sinal for the server
    int signal = 0;
    int status = sigwait(&signals, &signal);
    if (status == 0)
    {
        std::cout << "received signal " << signal << std::endl;
    }
    else
    {
        std::cerr << "sigwait returns " << status << std::endl;
    }

    stats.stop();
}