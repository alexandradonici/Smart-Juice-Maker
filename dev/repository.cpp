// Declare all helper files
const string FRUITS_CALORIES_DB = "./storage/fruits_calories.json";
const string FRUITS_VITAMINS_DB = "./storage/fruits_vitamins.json";
const string JUICE_HISTORY_DB = "./storage/juices.json";
const string FRUITS_CURRENT = "./storage/current_fruits.json";

json readJson(string fileName)
 {
    ifstream read(fileName);
    json in;
    read >> in;
    return in;
 }

vector<FruitCalories> GetFruitCalories()
{
    json in = readJson(FRUITS_CALORIES_DB);
    auto fruitCalories = in.get<vector<FruitCalories>>();  
    return fruitCalories;
}

vector<Juice> GetJuiceHistory()
{
    json juiceHistoryJson = readJson(JUICE_HISTORY_DB);
    vector<Juice> juices = juiceHistoryJson.get<vector<Juice>>();

    return juices;
}

void AddJuiceInHistory(vector<Juice> juiceHistory, Juice newJuice)
{
    json juice(newJuice);

    juiceHistory.push_back(newJuice);
    json juicesJson = juiceHistory;

    ofstream out(JUICE_HISTORY_DB);
    out << juicesJson;
}

vector<VitaminFruits> GetVitaminFruits()
{
    json in = readJson(FRUITS_VITAMINS_DB);
    auto vitaminFruits = in.get<vector<VitaminFruits>>();  
    return vitaminFruits;
}

json GetCurrentFruits() {
    json jsonFruits = readJson(FRUITS_CURRENT);
    return jsonFruits;
}