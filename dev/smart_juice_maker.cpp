/*
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
#include <chrono>
#include <time.h>
#include <string>
#include <signal.h>
#include <mosquitto.h>
#include "unistd.h"
#include "models.cpp"
#include "repository.cpp"

// General advice: pay attention to the namespaces that you use in various contexts. Could prevent headaches.
using namespace std;
using namespace Pistache;

// Helper class
class Helpers
{
public:
    const static string currentDateTime();
    const static time_t getTimestampFromString(const string &dateTimeString);
    static void removeStringDuplicates(vector<string> &v);
};

const string Helpers::currentDateTime()
{
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

const time_t Helpers::getTimestampFromString(const string &dateTimeString) {
    const char *dateTime = dateTimeString.data();
    struct tm tm = { 0 };
    strptime(dateTime, "%Y-%m-%d.%X", &tm);
    return mktime(&tm);
}

 void Helpers::removeStringDuplicates(vector<string> &v)
{
    vector<string>::iterator itr = v.begin();
    unordered_set<string> s;
 
    for (auto curr = v.begin(); curr != v.end(); ++curr)
    {
        if (s.insert(*curr).second) {
            *itr++ = *curr;
        }
    }
 
    v.erase(itr, v.end());
}

namespace Generic
{
    const int HTTP = 0;
    const int MQTT = 1;

    void handleReady(const Rest::Request &, Http::ResponseWriter response)
    {
        response.send(Http::Code::Ok, "UP\n");
    }

    vector<string> GetFruitNames(vector<Fruit> fruits)
    {
        vector<string> fruitNames;
        
        for(auto fruit : fruits)
            fruitNames.push_back(fruit.getName());

        return fruitNames;
    }

    vector<string> GetVitaminsByFruits(vector<string> clientFruits)
    {
        auto vitaminFruits = GetVitaminFruits();
        vector<string> vitamins;

        for(auto vitamin : vitaminFruits)
        {
            for(auto fruit : vitamin.getFruits())
            {
                for(auto clientFruit : clientFruits)
                {
                    if(boost::iequals(clientFruit, fruit))
                    {
                        vitamins.push_back(vitamin.getVitaminName());
                        break;
                    }
                }
            }
        }

        return vitamins;
    }

    json createJuice(json clientJson, int reqType) {
        auto clientFruits = clientJson.get<vector<Fruit>>();
        auto fruitCalories = GetFruitCalories();
        auto juices = GetJuiceHistory();

        Juice newJuice;

        newJuice.setIdentifier(juices.size() + 1);
        newJuice.setQuantity(0);
        newJuice.setCalories(0);
        newJuice.setPreparationDate(Helpers::currentDateTime());

        for (auto clientFruit : clientFruits)
        {
            if(clientFruit.getQuantity() < 0) 
            {
                if(reqType == HTTP) 
                {
                    ErrorHTTP error(Http::Code::Bad_Request, "The quantity must be positive!");
                    json jsonErrorHttp(error);
                    return jsonErrorHttp;
                } else 
                {
                    ErrorMQTT error("The quantity must be positive!");
                    json jsonError(error);
                    return jsonError;
                }
            }
            bool fruitInFile = false;
            for (auto fruitCal : fruitCalories)
            {
                if (boost::iequals(clientFruit.getName(), fruitCal.getName()))
                {   
                    auto fruits = newJuice.getFruits();
                    fruits.push_back(clientFruit.getName());
                    newJuice.setFruits(fruits);
                    newJuice.setQuantity(newJuice.getQuantity() + clientFruit.getQuantity());
                    newJuice.setCalories(newJuice.getCalories() + (fruitCal.getCalories() * clientFruit.getQuantity()) / 100);
                    fruitInFile = true; // Check if the fruit is in the calories database
                }
            }
            if(!fruitInFile) 
            {
                if(reqType == HTTP) 
                {
                    ErrorHTTP error(Http::Code::Not_Found, "The fruit " + clientFruit.getName() + " cannot be found in the database!");
                    json jsonErrorHttp(error);
                    return jsonErrorHttp;
                } else 
                {
                    ErrorMQTT error("The fruit " + clientFruit.getName() + " cannot be found in the database!");
                    json jsonError(error);
                    return jsonError;
                }
            }
        }

        vector<string> vitamins = GetVitaminsByFruits(GetFruitNames(clientFruits));
        Helpers::removeStringDuplicates(vitamins);
        newJuice.setVitamins(vitamins);

        json juice(newJuice);
        AddJuiceInHistory(juices, newJuice);

        return juice;
    }

    void makeJuice(const Rest::Request &request, Http::ResponseWriter response)
    {
        // from client
        auto clientJson = nlohmann::json::parse(request.body());
        json juice = createJuice(clientJson, Generic::HTTP);
        response.send(Http::Code::Ok, juice.dump());
    }

    // second functionality, get a list of fruits based on a list of vitamins from client
    void getFruitsByVitamins(const Rest::Request &request, Http::ResponseWriter response)
    {
        auto clientJson = nlohmann::json::parse(request.body());
        auto clientVitamins = clientJson.get<vector<string>>();

        auto vitaminFruits = GetVitaminFruits();

        std::vector<VitaminFruits> associatedFruits;

        for(auto clientVitamin : clientVitamins)
        {
            for(auto vitamin : vitaminFruits)
            {
                if(boost::iequals(clientVitamin, vitamin.getVitaminName()))
                {
                    associatedFruits.push_back(vitamin);
                }
            }
        }
        response.send(Http::Code::Ok, json(associatedFruits).dump());
    }

    // the third functionality: get all juices between two given dates
    // if one of the dates is missing, it will be ignored
    void getJuicesBetweenDates(const Rest::Request &request, Http::ResponseWriter response) 
    {
        const auto clientJson = nlohmann::json::parse(request.body());
        time_t dateFrom = 0, dateTo = numeric_limits<time_t>::max();

        if (clientJson.contains("dateFrom")) 
        {
            dateFrom = Helpers::getTimestampFromString(clientJson["dateFrom"].get<string>());
        }
        if (clientJson.contains("dateTo")) 
        {
            dateTo = Helpers::getTimestampFromString(clientJson["dateTo"].get<string>());
        }
        
        const auto juices = GetJuiceHistory();
        vector<Juice> filteredJuices;
        
        for (auto juice : juices) 
        {
            auto preparationDate = Helpers::getTimestampFromString(juice.getPreparationDate());

            if (difftime(preparationDate, dateFrom) > 0 && difftime(preparationDate, dateTo) < 0)
            {
                filteredJuices.push_back(juice);
            }
        }  

        response.send(Http::Code::Ok, json(filteredJuices).dump());
    }

    // the fourth functionality: get a juice by identifier
    void getJuiceByIdentifier(const Rest::Request &request, Http::ResponseWriter response) 
    {
        const auto clientJson = nlohmann::json::parse(request.body());
        int identifier = clientJson["identifier"].get<int>();

        const auto juices = GetJuiceHistory();
        Juice *desiredJuice = NULL; 

        for (auto juice : juices) 
        {
            if (identifier == juice.getIdentifier())
            {
                desiredJuice = new Juice(juice);
                break;
            }
        }
        
        if (desiredJuice != NULL) 
        {
            response.send(Http::Code::Ok, json(*(desiredJuice)).dump());  
            return;
        } 
        response.send(Http::Code::Not_Found, "Juice not found!"); 
    }

    // get a list of fruits based on the amount of calories the juice will contain
    void getQuantitiesByCaloriesAndFruits(const Rest::Request &request, Http::ResponseWriter response)
    {
        const auto clientJson = nlohmann::json::parse(request.body());
        double calories = clientJson["calories"].get<int>();
        auto clientFruits = clientJson["fruits"].get<vector<Fruit>>();
        auto calSum = 0;
        
        auto fruitCalories = GetFruitCalories();
        
        for (auto clientFruit : clientFruits) 
        {
            if(clientFruit.getQuantity() < 0) 
            {
                response.send(Http::Code::Bad_Request, "The quantity must be positive!");   
            }
            bool fruitInFile = false;
            for (auto fruitCal : fruitCalories) 
            {
                if (boost::iequals(clientFruit.getName(), fruitCal.getName()))
                {
                    auto newCal = (clientFruit.getQuantity() * fruitCal.getCalories()) / 100;
                    calSum += newCal;
                    fruitInFile = true; // Check if the fruit is in the calories database
                }
            }
            if(!fruitInFile) 
            {
                response.send(Http::Code::Not_Found, "The fruit " + clientFruit.getName() + 
                                                    " cannot be found in the database!");                 
            }
        }

        if (calSum < calories) 
        {
            response.send(Http::Code::Not_Found, "Insufficient num of calories!"); 
        } else 
        {
            vector<Fruit> newFruits;
            for (auto clientFruit : clientFruits) 
            {
                auto newQuantity = (clientFruit.getQuantity() * calories) / calSum;
                Fruit newFruit;
                newFruit.setName(clientFruit.getName());
                // Round the results to 2 points decimal
                newFruit.setQuantity(round( newQuantity * 100.0 ) / 100.0);
                newFruits.push_back(newFruit);
            }
            
            response.send(Http::Code::Ok, json(newFruits).dump());
        }
    }
}

// Definition of the SmartJuiceMakerEndpoint class
class SmartJuiceMakerEndpoint
{
public:
    explicit SmartJuiceMakerEndpoint(Address addr)
        : httpEndpoint(make_shared<Http::Endpoint>(addr))
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
        Routes::Get(router, "/getJuicesBetweenDates", Routes::bind(&Generic::getJuicesBetweenDates));
        Routes::Get(router, "/getJuiceByIdentifier", Routes::bind(&Generic::getJuiceByIdentifier));
        Routes::Post(router, "/getQuantitiesByCaloriesAndFruits", Routes::bind(&Generic::getQuantitiesByCaloriesAndFruits));
    }

    // Defining the class of the SmartJuiceMaker. It should model the entire configuration of the SmartJuiceMaker
    class SmartJuiceMaker
    {
    public:
        explicit SmartJuiceMaker() {}
    };

    // Create the lock which prevents concurrent editing of the same variable
    using Lock = mutex;
    using Guard = lock_guard<Lock>;
    Lock smartJuiceMakerLock;

    // Instance of the smart juice maker model
    SmartJuiceMaker sjm;

    // Defining the httpEndpoint and a router.
    shared_ptr<Http::Endpoint> httpEndpoint;
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
        port = static_cast<uint16_t>(stol(argv[1]));

        if (argc == 3)
            thr = stoi(argv[2]);
    }

    Address addr(Ipv4::any(), port);

    cout << "Cores = " << hardware_concurrency() << endl;
    cout << "Using " << thr << " threads" << endl;

    // Instance of the class that defines what the server can do.
    SmartJuiceMakerEndpoint stats(addr);

    // Initialize and start the server
    stats.init(thr);
    stats.start();

    struct mosquitto *mosq;
    // Fork so we'll have 2 processes: one for http requests and for mqtt
    int pid = fork();
    if(pid == 0) {
        
        mosquitto_lib_init();

        mosq = mosquitto_new("publisher", true, NULL);
        int rc = mosquitto_connect(mosq, "localhost", 1883, 60);
        if(rc != 0)
        {
            printf("\nError with code = %d\n", rc);
            mosquitto_destroy(mosq);
            return -1;
        }
        while(1)
        {
            json juice = Generic::createJuice(GetCurrentFruits(), Generic::MQTT);
            string stringJuice = juice.dump();
            mosquitto_publish(mosq, NULL, "mqtt", stringJuice.size(), stringJuice.c_str(), 0, false);
            sleep(20);
        }
    }

    // Code that waits for the shutdown sinal for the server
    int signal = 0;
    int status = sigwait(&signals, &signal);
    if (status == 0)
    {
        mosquitto_disconnect(mosq);
        mosquitto_lib_cleanup();
        mosquitto_destroy(mosq);
        cout << "received signal " << signal << endl;
    }
    else
    {
        cerr << "sigwait returns " << status << endl;
    }

    stats.stop();
}