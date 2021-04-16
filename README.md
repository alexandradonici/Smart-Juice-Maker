# Smart-Juice-Maker

Raportul de analiza poate fi accesat [aici](https://drive.google.com/file/d/1E63-G4GlCUnH10iFmfEvAfgfBoLkKwqw/view?usp=sharing).

## Setup

### Install Pistache
```
sudo add-apt-repository ppa:pistache+team/unstable
sudo apt update
sudo apt install libpistache-dev
```

### Install Nlohmann
```
sudo apt-get install nlohmann-json3-dev
```

### Install g++
```
sudo apt install g++
```

### Install Boost
```
sudo apt-get install libboost-dev
```

### Compile the file
```
 g++ smart_juice_maker.cpp -o smartjuicemaker -lpistache -lcrypto -lssl -lpthread -std=c++17
```

### Run the file
```
 ./smartjuicemaker
 ```

## Features

1. ### Get a juice based on a list of fruits
```
http://localhost:9080/makeJuice
```
Input format:
```
[
{"fruit": "orange", "quantity": 100},
{"fruit": "bananas", "quantity":200},
{"fruit": "peaches", "quantity":100}
]
```
Output format:
```
{
  "calories": 180.0,
  "fruits": [
    "orange",
    "bananas",
    "peaches"
  ],
  "identifier": 29,
  "preparationDate": "2021-04-15.23:06:04",
  "quantity": 400.0,
  "vitamins": [
    "vitamin C",
    "vitamin K",
    "vitamin E",
    "vitamin B6"
  ]
}
```

2. ### Get a list of fruits based on a list of vitamins from client
```
localhost:9080/getFruitsByVitamins
```
Input format:
```
["vitamin A", "vitamin B"]
```
Output format:
```
[
  "mango",
  "cantaloupe",
  "grapefruit",
  "watermelon",
  "papaya",
  "carrot",
  "apricot",
  "passion fruit",
  "cherries",
  "salad"
]
```
3. ### Get all juices between two given dates
```
localhost:9080/getJuicesBetweenDates
```
Input format:
```
{
    "dateFrom": "2021-03-17.20:01:00",
    "dateTo": "2021-04-17.20:01:00"
}
```
Output format:
```
[
  {
    "calories": 149.5,
    "fruits": [
      "orange",
      "bananas",
      "salad"
    ],
    "identifier": 1,
    "preparationDate": "2021-04-13.19:26:43",
    "quantity": 350.0,
    "vitamins": [
      "vitamin A",
      "vitamin C",
      "vitamin B6"
    ]
  },
  {
    "calories": 149.5,
    "fruits": [
      "orange",
      "bananas",
      "salad"
    ],
    "identifier": 2,
    "preparationDate": "2021-04-13.19:38:56",
    "quantity": 350.0,
    "vitamins": [
      "vitamin A",
      "vitamin C",
      "vitamin B6"
    ]
  }
]
```

4. ### Get a juice by identifier
```
localhost:9080/getJuiceByIdentifier
```
Input format:
```
{"identifier": 1}
```
Output format:
```
{
  "calories": 149.5,
  "fruits": [
    "orange",
    "bananas",
    "salad"
  ],
  "identifier": 1,
  "preparationDate": "2021-04-13.19:26:43",
  "quantity": 350.0,
  "vitamins": [
    "vitamin A",
    "vitamin C",
    "vitamin B6"
  ]
}
```
5. ### Given the desired number of calories of the juice and a list of fruits, return the quantity of each fruit depending on the total number of calories
```
localhost:9080/getQuantitiesByCaloriesAndFruits
```
Input format:
```
{
"calories": 100,
"fruits": [{"fruit":"mango", "quantity": 100},
{"fruit":"cantaloupe", "quantity":300},
{"fruit":"grapefruit", "quantity":150}]
}
```
Output format:
```
[
  {
    "fruit": "mango",
    "quantity": 49.26
  },
  {
    "fruit": "cantaloupe",
    "quantity": 147.78
  },
  {
    "fruit": "grapefruit",
    "quantity": 73.89
  }
]
```

