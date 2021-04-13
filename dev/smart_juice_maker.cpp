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
#include <string>
#include <signal.h>

using namespace std;
using namespace Pistache;
using json = nlohmann::json;

// Declare all helper files
const std::string caloriesFile = "fruits_calories.json";
const std::string vitaminsFile = "fruits_vitamins.json";
const std::string juiceHistoryDB = "juices.json";

// Helper class
class Helpers
{
public:
    const static std::string currentDateTime();
    static json readJson(std::string fileName);
    static void removeStringDuplicates(std::vector<string> &v);


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

 json Helpers::readJson(std::string fileName)
 {
    std::ifstream read(fileName);
    json in;
    read >> in;
    return in;
 }

 void Helpers::removeStringDuplicates(std::vector<string> &v)
{
    std::vector<string>::iterator itr = v.begin();
    std::unordered_set<string> s;
 
    for (auto curr = v.begin(); curr != v.end(); ++curr)
    {
        if (s.insert(*curr).second) {
            *itr++ = *curr;
        }
    }
 
    v.erase(itr, v.end());
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

class Fruit
{
public:
    std::string name;
    double quantity;
    std::vector<string> vitamins;
};

class FruitCalories
{
public:
    std::string name;
    double calories;
};

class VitaminFruits
{
    public:
    std::string vitaminName;
    std::vector<std::string> fruits;
};

class Juice
{
public:
    int identifier;
    double calories;
    double quantity;
    string preparationDate;
    std::vector<string> vitamins;
    std::vector<string> fruits;
};

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

void from_json(const json &json, VitaminFruits &vitaminFruits)
{
    json.at("vitamin").get_to(vitaminFruits.vitaminName);
    json.at("fruits").get_to(vitaminFruits.fruits);
}

void from_json(const json &json, Juice &juice)
{
     json.at("identifier").get_to(juice.identifier);
     json.at("quantity").get_to(juice.quantity);
     json.at("calories").get_to(juice.calories);
     json.at("preparationDate").get_to(juice.preparationDate);
     json.at("fruits").get_to(juice.fruits);
    json.at("vitamins").get_to(juice.vitamins);
}

// to_json overloading
void to_json(json &j, const Juice &juice)
{
    j = json{
        {"identifier", juice.identifier},
        {"calories", juice.calories},
        {"quantity", juice.quantity},
        {"preparationDate", juice.preparationDate},
        {"fruits", juice.fruits},
        {"vitamins", juice.vitamins}};
}

// Some generic namespace, with a simple function we could use to test the creation of the endpoints.
namespace Generic
{
    void handleReady(const Rest::Request &, Http::ResponseWriter response)
    {
        response.send(Http::Code::Ok, "1");
    }

      std::vector<FruitCalories> GetFruitCalories()
    {
        json in = Helpers::readJson(caloriesFile);
        auto fruitCalories = in.get<std::vector<FruitCalories>>();  
        return fruitCalories;
    }

    std::vector<Juice> GetJuiceHistory()
    {
        json juiceHistoryJson = Helpers::readJson(juiceHistoryDB);
        std::vector<Juice> juices = juiceHistoryJson.get<std::vector<Juice>>();

        return juices;
    }

    void AddJuiceInHistory(std::vector<Juice> juiceHistory, Juice newJuice)
    {
        json juice(newJuice);

        juiceHistory.push_back(newJuice);
        json juicesJson = juiceHistory;

        std::ofstream out(juiceHistoryDB);
        out << juicesJson;
    }

    std::vector<VitaminFruits> GetVitaminFruits()
    {
        json in = Helpers::readJson(vitaminsFile);
        auto vitaminFruits = in.get<std::vector<VitaminFruits>>();  
        return vitaminFruits;
    }

    std::vector<std::string> GetFruitNames(std::vector<Fruit> fruits)
    {
        std::vector<std::string> fruitNames;
        
        for(auto fruit : fruits)
            fruitNames.push_back(fruit.name);

            return fruitNames;
    }

    std::vector<string> GetVitaminsByFruits(std::vector<string> clientFruits)
    {
        auto vitaminFruits = GetVitaminFruits();
        std::vector<std::string> vitamins;

        for(auto vitamin : vitaminFruits)
        {
            for(auto fruit : vitamin.fruits)
            {
                for(auto clientFruit : clientFruits)
                {
                    if(boost::iequals(clientFruit, fruit))
                    {
                        vitamins.push_back(vitamin.vitaminName);
                        break;
                    }
                }
            }
        }

        return vitamins;
    }

    void makeJuice(const Rest::Request &request, Http::ResponseWriter response)
    {
        // from client
        auto clientJson = nlohmann::json::parse(request.body());
        auto clientFruits = clientJson.get<std::vector<Fruit>>();
        auto fruitCalories = GetFruitCalories();
        auto juices = GetJuiceHistory();

        Juice newJuice;

        newJuice.identifier = juices.size() + 1;
        newJuice.quantity = 0;
        newJuice.calories = 0;
        newJuice.preparationDate = Helpers::currentDateTime();

        for (auto fruitClient : clientFruits)
        {
            for (auto fruitCal : fruitCalories)
            {
                if (boost::iequals(fruitClient.name, fruitCal.name))
                {
                    newJuice.fruits.push_back(fruitClient.name);
                    newJuice.quantity += fruitClient.quantity;
                    newJuice.calories += (fruitCal.calories * fruitClient.quantity) / 100;
                }
            }
        }

        newJuice.vitamins = GetVitaminsByFruits(GetFruitNames(clientFruits));
        Helpers::removeStringDuplicates(newJuice.vitamins);

        json juice(newJuice);
        AddJuiceInHistory(juices, newJuice);
       
        response.send(Http::Code::Ok, juice.dump());
    }

    //second functionality, get a list of fruits based on a list of vitamins from client
    void getFruitsByVitamins(const Rest::Request &request, Http::ResponseWriter response)
    {
        auto clientJson = nlohmann::json::parse(request.body());
        auto clientVitamins = clientJson.get<std::vector<std::string>>();

        auto vitaminFruits = GetVitaminFruits();

        std::vector<std::string> fruits;

        for(auto clientVitamin : clientVitamins)
        {
            for(auto vitamin : vitaminFruits)
            {
                if(boost::iequals(clientVitamin, vitamin.vitaminName))
                {
                    fruits.insert( fruits.end(), vitamin.fruits.begin(), vitamin.fruits.end());
                }
            }
        }

        Helpers::removeStringDuplicates(fruits);

        json fruitsJson(fruits);

        response.send(Http::Code::Ok, fruitsJson.dump());

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
        Routes::Post(router, "/getFruitsByVitamins", Routes::bind(&Generic::getFruitsByVitamins));
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