#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

class Fruit
{
private:
    string name;
    double quantity;
    vector<string> vitamins;
public:
    void setName(string name)
    {
        this->name = name;
    }

    string getName() const
    {
        return name;
    }

    void setQuantity(double quantity)
    {
        this->quantity = quantity;
    }

    double getQuantity() const
    {
        return quantity;
    }

    void setVitamins(vector<string> vitamins)
    {
        this->vitamins = vitamins;
    }

    vector<string> getVitamins()
    {
        return vitamins;
    }
};

class FruitCalories
{
private:
    string name;
    double calories;
public:
    void setName(string name)
    {
        this->name = name;
    }

    string getName()
    {
        return name;
    }

    void setCalories(double calories)
    {
        this->calories = calories;
    }

    double getCalories()
    {
        return calories;
    }
};

class VitaminFruits
{
private:
    string vitaminName;
    vector<string> fruits;
public:
    void setVitaminName(string vitaminName)
    {
        this->vitaminName = vitaminName;
    }

    string getVitaminName() const
    {
        return vitaminName;
    }

    void setFruits(vector<string> fruits)
    {
        this->fruits = fruits;
    }
 
    vector<string> getFruits() const
    {
        return fruits;
    }
};

class Juice
{
private:
    int identifier;
    double calories;
    double quantity;
    string preparationDate;
    vector<string> vitamins;
    vector<string> fruits;
public:
    void setIdentifier(int identifier)
    {
        this->identifier = identifier;
    }

    int getIdentifier() const
    {
        return identifier;
    }

    void setCalories(double calories)
    {
        this->calories = calories;
    }

    double getCalories() const
    {
        return calories;
    }

    void setQuantity(double quantity)
    {
        this->quantity = quantity;
    }

    double getQuantity() const
    {
        return quantity;
    }

    void setPreparationDate(string preparationDate)
    {
        this->preparationDate = preparationDate;
    }

    string getPreparationDate() const
    {
        return preparationDate;
    }

    void setVitamins(vector<string> vitamins)
    {
        this->vitamins = vitamins;
    }

    vector<string> getVitamins() const
    {
        return vitamins;
    }

    void setFruits(vector<string> fruits)
    {
        this->fruits = fruits;
    }

    vector<string> getFruits() const
    {
        return fruits;
    }
};

class ErrorHTTP
{
private:
    string error;
    Pistache::Http::Code code;    
public:
    ErrorHTTP(Pistache::Http::Code code, string error) 
    {
        this->code = code;
        this->error = error;
    }

    void setError(string error) 
    {
        this->error = error;
    }

    void setCode(Pistache::Http::Code code) 
    {
        this->code = code;
    }

    string getError() const
    {
        return error;
    }

    Pistache::Http::Code getCode() const 
    {
        return code;
    }
};

class ErrorMQTT
{
private:
    string error;
public:
    ErrorMQTT(string error) 
    {
        this->error = error;
    }

    void setError(string error) 
    {
        this->error = error;
    }

    string getError() const
    {
        return error;
    }
};

// from_json overloading
void from_json(const json &json, Fruit &fruit)
{
    fruit.setName(json.at("fruit"));
    fruit.setQuantity(json.at("quantity"));
}

void from_json(const json &json, FruitCalories &fruitCalories)
{
    fruitCalories.setName(json.at("fruit"));
    fruitCalories.setCalories(json.at("calories"));
}

void from_json(const json &json, VitaminFruits &vitaminFruits)
{
    vitaminFruits.setVitaminName(json.at("vitamin"));
    vitaminFruits.setFruits(json.at("fruits"));
}

void from_json(const json &json, Juice &juice)
{
    juice.setIdentifier(json.at("identifier"));
    juice.setQuantity(json.at("quantity"));
    juice.setCalories(json.at("calories"));
    juice.setPreparationDate(json.at("preparationDate"));
    juice.setFruits(json.at("fruits"));
    juice.setVitamins(json.at("vitamins"));
}

void from_json(const json &json, ErrorHTTP &error) 
{
    error.setCode(json.at("code"));
    error.setError(json.at("error"));
}

void from_json(const json &json, ErrorMQTT &error) 
{
    error.setError(json.at("error"));
}

// to_json overloading
void to_json(json &j, const Fruit &fruit)
{
    j = json{
        {"fruit", fruit.getName()},
        {"quantity", fruit.getQuantity()}};
}

void to_json(json &j, const Juice &juice)
{
    j = json{
        {"identifier", juice.getIdentifier()},
        {"calories", juice.getCalories()},
        {"quantity", juice.getQuantity()},
        {"preparationDate", juice.getPreparationDate()},
        {"fruits", juice.getFruits()},
        {"vitamins", juice.getVitamins()}};
}

void to_json(json &j, const VitaminFruits &vitaminFruits)
{
    j = json{
        {"vitamin", vitaminFruits.getVitaminName()},
        {"fruits", vitaminFruits.getFruits()}
    };
}
void to_json(json &j, const ErrorHTTP &error)
{
    j = json{
        {"error", error.getError()},
        {"code", error.getCode()}};
}

void to_json(json &j, const ErrorMQTT &error)
{
    j = json{
        {"error", error.getError()}};
}
