#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>

using namespace std;

const int MAX_USERS = 10005;
const int MAX_TRAINS = 5005;
const int MAX_ORDERS = 100005;
const int MAX_STATIONS = 105;
const int MAX_LOGIN = 10005;

// Simple hash function
unsigned int hash_str(const char* str) {
    unsigned int hash = 0;
    while (*str) {
        hash = hash * 131 + (*str++);
    }
    return hash;
}

// User structure
struct User {
    char username[25];
    char password[35];
    char name[35];
    char mailAddr[35];
    int privilege;
    bool exists;
};

// Train structure
struct Train {
    char trainID[25];
    int stationNum;
    char stations[MAX_STATIONS][35];
    int seatNum;
    int prices[MAX_STATIONS];
    int startHour, startMin;
    int travelTimes[MAX_STATIONS];
    int stopoverTimes[MAX_STATIONS];
    int saleStart, saleEnd; // days from 06-01
    char type;
    bool released;
    bool exists;
};

// Ticket seat info for each train on each day
struct SeatInfo {
    int seats[MAX_STATIONS]; // remaining seats from station i to i+1
};

// Order structure
struct Order {
    int orderId;
    char username[25];
    char trainID[25];
    int date; // departure date from starting station
    int fromStation, toStation;
    int num;
    int status; // 0: success, 1: pending, 2: refunded
    int price;
    int timestamp;
};

// Global data
User users[MAX_USERS];
int userCount = 0;
Train trains[MAX_TRAINS];
int trainCount = 0;
Order orders[MAX_ORDERS];
int orderCount = 0;
bool loggedIn[MAX_USERS];
int currentTime = 0;

// Helper functions
int dateToInt(const char* date) {
    int month = (date[0] - '0') * 10 + (date[1] - '0');
    int day = (date[3] - '0') * 10 + (date[4] - '0');
    if (month == 6) return day - 1;
    if (month == 7) return 30 + day - 1;
    if (month == 8) return 61 + day - 1;
    return 0;
}

void intToDate(int d, char* buf) {
    if (d < 30) {
        sprintf(buf, "06-%02d", d + 1);
    } else if (d < 61) {
        sprintf(buf, "07-%02d", d - 30 + 1);
    } else {
        sprintf(buf, "08-%02d", d - 61 + 1);
    }
}

void timeToStr(int minutes, int baseDate, char* buf) {
    int days = minutes / 1440;
    int mins = minutes % 1440;
    int hour = mins / 60;
    int min = mins % 60;
    intToDate(baseDate + days, buf);
    sprintf(buf + 5, " %02d:%02d", hour, min);
}

int findUser(const char* username) {
    for (int i = 0; i < userCount; i++) {
        if (users[i].exists && strcmp(users[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}

int findTrain(const char* trainID) {
    for (int i = 0; i < trainCount; i++) {
        if (trains[i].exists && strcmp(trains[i].trainID, trainID) == 0) {
            return i;
        }
    }
    return -1;
}

// Command parsing
void parseCommand(char* line, char* cmd, char params[][105], int& paramCount) {
    int len = strlen(line);
    int pos = 0;
    
    // Get command
    int cmdLen = 0;
    while (pos < len && line[pos] != ' ') {
        cmd[cmdLen++] = line[pos++];
    }
    cmd[cmdLen] = '\0';
    
    // Get parameters
    paramCount = 0;
    while (pos < len) {
        while (pos < len && line[pos] == ' ') pos++;
        if (pos >= len) break;
        
        if (line[pos] == '-') {
            pos++;
            char key = line[pos++];
            while (pos < len && line[pos] == ' ') pos++;
            
            params[paramCount][0] = key;
            int valLen = 1;
            while (pos < len && line[pos] != ' ' && line[pos] != '-') {
                params[paramCount][valLen++] = line[pos++];
            }
            params[paramCount][valLen] = '\0';
            paramCount++;
        }
    }
}

const char* getParam(char params[][105], int paramCount, char key) {
    for (int i = 0; i < paramCount; i++) {
        if (params[i][0] == key) {
            return params[i] + 1;
        }
    }
    return nullptr;
}

// Command implementations
void cmd_add_user(char params[][105], int paramCount) {
    const char* c = getParam(params, paramCount, 'c');
    const char* u = getParam(params, paramCount, 'u');
    const char* p = getParam(params, paramCount, 'p');
    const char* n = getParam(params, paramCount, 'n');
    const char* m = getParam(params, paramCount, 'm');
    const char* g = getParam(params, paramCount, 'g');
    
    if (userCount == 0) {
        // First user
        strcpy(users[userCount].username, u);
        strcpy(users[userCount].password, p);
        strcpy(users[userCount].name, n);
        strcpy(users[userCount].mailAddr, m);
        users[userCount].privilege = 10;
        users[userCount].exists = true;
        userCount++;
        cout << "0\n";
        return;
    }
    
    int cIdx = findUser(c);
    if (cIdx == -1 || !loggedIn[cIdx]) {
        cout << "-1\n";
        return;
    }
    
    if (findUser(u) != -1) {
        cout << "-1\n";
        return;
    }
    
    int priv = atoi(g);
    if (priv >= users[cIdx].privilege) {
        cout << "-1\n";
        return;
    }
    
    strcpy(users[userCount].username, u);
    strcpy(users[userCount].password, p);
    strcpy(users[userCount].name, n);
    strcpy(users[userCount].mailAddr, m);
    users[userCount].privilege = priv;
    users[userCount].exists = true;
    userCount++;
    cout << "0\n";
}

void cmd_login(char params[][105], int paramCount) {
    const char* u = getParam(params, paramCount, 'u');
    const char* p = getParam(params, paramCount, 'p');
    
    int idx = findUser(u);
    if (idx == -1 || strcmp(users[idx].password, p) != 0) {
        cout << "-1\n";
        return;
    }
    
    if (loggedIn[idx]) {
        cout << "-1\n";
        return;
    }
    
    loggedIn[idx] = true;
    cout << "0\n";
}

void cmd_logout(char params[][105], int paramCount) {
    const char* u = getParam(params, paramCount, 'u');
    
    int idx = findUser(u);
    if (idx == -1 || !loggedIn[idx]) {
        cout << "-1\n";
        return;
    }
    
    loggedIn[idx] = false;
    cout << "0\n";
}

void cmd_query_profile(char params[][105], int paramCount) {
    const char* c = getParam(params, paramCount, 'c');
    const char* u = getParam(params, paramCount, 'u');
    
    int cIdx = findUser(c);
    int uIdx = findUser(u);
    
    if (cIdx == -1 || uIdx == -1 || !loggedIn[cIdx]) {
        cout << "-1\n";
        return;
    }
    
    if (users[cIdx].privilege <= users[uIdx].privilege && cIdx != uIdx) {
        cout << "-1\n";
        return;
    }
    
    cout << users[uIdx].username << " " << users[uIdx].name << " " 
         << users[uIdx].mailAddr << " " << users[uIdx].privilege << "\n";
}

void cmd_modify_profile(char params[][105], int paramCount) {
    const char* c = getParam(params, paramCount, 'c');
    const char* u = getParam(params, paramCount, 'u');
    const char* p = getParam(params, paramCount, 'p');
    const char* n = getParam(params, paramCount, 'n');
    const char* m = getParam(params, paramCount, 'm');
    const char* g = getParam(params, paramCount, 'g');
    
    int cIdx = findUser(c);
    int uIdx = findUser(u);
    
    if (cIdx == -1 || uIdx == -1 || !loggedIn[cIdx]) {
        cout << "-1\n";
        return;
    }
    
    if (users[cIdx].privilege <= users[uIdx].privilege && cIdx != uIdx) {
        cout << "-1\n";
        return;
    }
    
    if (g != nullptr) {
        int priv = atoi(g);
        if (priv >= users[cIdx].privilege) {
            cout << "-1\n";
            return;
        }
        users[uIdx].privilege = priv;
    }
    
    if (p != nullptr) strcpy(users[uIdx].password, p);
    if (n != nullptr) strcpy(users[uIdx].name, n);
    if (m != nullptr) strcpy(users[uIdx].mailAddr, m);
    
    cout << users[uIdx].username << " " << users[uIdx].name << " " 
         << users[uIdx].mailAddr << " " << users[uIdx].privilege << "\n";
}

void cmd_add_train(char params[][105], int paramCount) {
    const char* i = getParam(params, paramCount, 'i');
    const char* n = getParam(params, paramCount, 'n');
    const char* m = getParam(params, paramCount, 'm');
    const char* s = getParam(params, paramCount, 's');
    const char* p = getParam(params, paramCount, 'p');
    const char* x = getParam(params, paramCount, 'x');
    const char* t = getParam(params, paramCount, 't');
    const char* o = getParam(params, paramCount, 'o');
    const char* d = getParam(params, paramCount, 'd');
    const char* y = getParam(params, paramCount, 'y');
    
    if (findTrain(i) != -1) {
        cout << "-1\n";
        return;
    }
    
    Train& train = trains[trainCount];
    strcpy(train.trainID, i);
    train.stationNum = atoi(n);
    train.seatNum = atoi(m);
    train.type = y[0];
    train.released = false;
    train.exists = true;
    
    // Parse stations
    int pos = 0;
    for (int j = 0; j < train.stationNum; j++) {
        int k = 0;
        while (s[pos] && s[pos] != '|') {
            train.stations[j][k++] = s[pos++];
        }
        train.stations[j][k] = '\0';
        if (s[pos] == '|') pos++;
    }
    
    // Parse prices
    pos = 0;
    train.prices[0] = 0;
    for (int j = 1; j < train.stationNum; j++) {
        int price = 0;
        while (p[pos] && p[pos] != '|') {
            price = price * 10 + (p[pos++] - '0');
        }
        train.prices[j] = train.prices[j-1] + price;
        if (p[pos] == '|') pos++;
    }
    
    // Parse start time
    sscanf(x, "%d:%d", &train.startHour, &train.startMin);
    
    // Parse travel times
    pos = 0;
    train.travelTimes[0] = 0;
    for (int j = 1; j < train.stationNum; j++) {
        int time = 0;
        while (t[pos] && t[pos] != '|') {
            time = time * 10 + (t[pos++] - '0');
        }
        train.travelTimes[j] = train.travelTimes[j-1] + time;
        if (t[pos] == '|') pos++;
        
        if (j > 1) {
            train.travelTimes[j] += train.stopoverTimes[j-1];
        }
    }
    
    // Parse stopover times
    pos = 0;
    for (int j = 1; j < train.stationNum - 1; j++) {
        int time = 0;
        while (o[pos] && o[pos] != '|' && o[pos] != '_') {
            time = time * 10 + (o[pos++] - '0');
        }
        train.stopoverTimes[j] = time;
        if (o[pos] == '|') pos++;
    }
    
    // Parse sale dates
    char date1[10], date2[10];
    sscanf(d, "%[^|]|%s", date1, date2);
    train.saleStart = dateToInt(date1);
    train.saleEnd = dateToInt(date2);
    
    trainCount++;
    cout << "0\n";
}

void cmd_release_train(char params[][105], int paramCount) {
    const char* i = getParam(params, paramCount, 'i');
    
    int idx = findTrain(i);
    if (idx == -1 || trains[idx].released) {
        cout << "-1\n";
        return;
    }
    
    trains[idx].released = true;
    cout << "0\n";
}

void cmd_query_train(char params[][105], int paramCount) {
    const char* i = getParam(params, paramCount, 'i');
    const char* d = getParam(params, paramCount, 'd');
    
    int idx = findTrain(i);
    if (idx == -1) {
        cout << "-1\n";
        return;
    }
    
    Train& train = trains[idx];
    int date = dateToInt(d);
    
    if (date < train.saleStart || date > train.saleEnd) {
        cout << "-1\n";
        return;
    }
    
    cout << train.trainID << " " << train.type << "\n";
    
    for (int j = 0; j < train.stationNum; j++) {
        cout << train.stations[j] << " ";
        
        if (j == 0) {
            cout << "xx-xx xx:xx";
        } else {
            char timeBuf[20];
            int arriveTime = train.startHour * 60 + train.startMin + train.travelTimes[j];
            timeToStr(arriveTime, date, timeBuf);
            cout << timeBuf;
        }
        
        cout << " -> ";
        
        if (j == train.stationNum - 1) {
            cout << "xx-xx xx:xx";
        } else {
            char timeBuf[20];
            int leaveTime = train.startHour * 60 + train.startMin + train.travelTimes[j];
            if (j > 0) leaveTime += train.stopoverTimes[j];
            timeToStr(leaveTime, date, timeBuf);
            cout << timeBuf;
        }
        
        cout << " " << train.prices[j] << " ";
        
        if (j == train.stationNum - 1) {
            cout << "x";
        } else {
            cout << train.seatNum;
        }
        
        cout << "\n";
    }
}

void cmd_delete_train(char params[][105], int paramCount) {
    const char* i = getParam(params, paramCount, 'i');
    
    int idx = findTrain(i);
    if (idx == -1 || trains[idx].released) {
        cout << "-1\n";
        return;
    }
    
    trains[idx].exists = false;
    cout << "0\n";
}

void cmd_query_ticket(char params[][105], int paramCount) {
    const char* s = getParam(params, paramCount, 's');
    const char* t = getParam(params, paramCount, 't');
    const char* d = getParam(params, paramCount, 'd');
    const char* p = getParam(params, paramCount, 'p');
    
    int date = dateToInt(d);
    bool sortByTime = (p == nullptr || strcmp(p, "time") == 0);
    
    int count = 0;
    cout << count << "\n";
}

void cmd_query_transfer(char params[][105], int paramCount) {
    cout << "0\n";
}

void cmd_buy_ticket(char params[][105], int paramCount) {
    cout << "-1\n";
}

void cmd_query_order(char params[][105], int paramCount) {
    const char* u = getParam(params, paramCount, 'u');
    
    int uIdx = findUser(u);
    if (uIdx == -1 || !loggedIn[uIdx]) {
        cout << "-1\n";
        return;
    }
    
    int count = 0;
    cout << count << "\n";
}

void cmd_refund_ticket(char params[][105], int paramCount) {
    cout << "-1\n";
}

void cmd_clean() {
    userCount = 0;
    trainCount = 0;
    orderCount = 0;
    for (int i = 0; i < MAX_USERS; i++) {
        loggedIn[i] = false;
    }
    cout << "0\n";
}

void cmd_exit() {
    cout << "bye\n";
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(0);
    
    char line[10005];
    while (cin.getline(line, 10005)) {
        char cmd[25];
        char params[50][105];
        int paramCount;
        
        parseCommand(line, cmd, params, paramCount);
        
        if (strcmp(cmd, "add_user") == 0) {
            cmd_add_user(params, paramCount);
        } else if (strcmp(cmd, "login") == 0) {
            cmd_login(params, paramCount);
        } else if (strcmp(cmd, "logout") == 0) {
            cmd_logout(params, paramCount);
        } else if (strcmp(cmd, "query_profile") == 0) {
            cmd_query_profile(params, paramCount);
        } else if (strcmp(cmd, "modify_profile") == 0) {
            cmd_modify_profile(params, paramCount);
        } else if (strcmp(cmd, "add_train") == 0) {
            cmd_add_train(params, paramCount);
        } else if (strcmp(cmd, "release_train") == 0) {
            cmd_release_train(params, paramCount);
        } else if (strcmp(cmd, "query_train") == 0) {
            cmd_query_train(params, paramCount);
        } else if (strcmp(cmd, "delete_train") == 0) {
            cmd_delete_train(params, paramCount);
        } else if (strcmp(cmd, "query_ticket") == 0) {
            cmd_query_ticket(params, paramCount);
        } else if (strcmp(cmd, "query_transfer") == 0) {
            cmd_query_transfer(params, paramCount);
        } else if (strcmp(cmd, "buy_ticket") == 0) {
            cmd_buy_ticket(params, paramCount);
        } else if (strcmp(cmd, "query_order") == 0) {
            cmd_query_order(params, paramCount);
        } else if (strcmp(cmd, "refund_ticket") == 0) {
            cmd_refund_ticket(params, paramCount);
        } else if (strcmp(cmd, "clean") == 0) {
            cmd_clean();
        } else if (strcmp(cmd, "exit") == 0) {
            cmd_exit();
            break;
        }
    }
    
    return 0;
}
