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
    int trainIdx;
    int date; // days from 06-01
    int seats[MAX_STATIONS]; // remaining seats from station i to i+1
};

// Order structure
struct Order {
    int orderId;
    char username[25];
    char trainID[25];
    int date; // departure date from boarding station
    int fromStation, toStation;
    int num;
    int status; // 0: success, 1: pending, 2: refunded
    int price;
    int timestamp;
    int trainIdx;
};

// Ticket result for query
struct TicketResult {
    int trainIdx;
    int fromIdx, toIdx;
    int leaveTime, arriveTime;
    int price;
    int seat;
    int date; // departure date from starting station
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

// Seat tracking: [trainIdx][date] -> seats array
SeatInfo seatData[MAX_TRAINS * 100]; // trainIdx * 100 + date
int seatDataCount = 0;

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

// Get or create seat info for a train on a specific date
SeatInfo* getSeatInfo(int trainIdx, int date) {
    // Search for existing
    for (int i = 0; i < seatDataCount; i++) {
        if (seatData[i].trainIdx == trainIdx && seatData[i].date == date) {
            return &seatData[i];
        }
    }
    
    // Create new
    SeatInfo* info = &seatData[seatDataCount++];
    info->trainIdx = trainIdx;
    info->date = date;
    for (int i = 0; i < trains[trainIdx].stationNum - 1; i++) {
        info->seats[i] = trains[trainIdx].seatNum;
    }
    return info;
}

// Find station index in train
int findStation(Train& train, const char* station) {
    for (int i = 0; i < train.stationNum; i++) {
        if (strcmp(train.stations[i], station) == 0) {
            return i;
        }
    }
    return -1;
}

// Compare function for sorting tickets
int compareTickets(const void* a, const void* b, bool byTime) {
    TicketResult* ta = (TicketResult*)a;
    TicketResult* tb = (TicketResult*)b;
    
    if (byTime) {
        int timeA = ta->arriveTime - ta->leaveTime;
        int timeB = tb->arriveTime - tb->leaveTime;
        if (timeA != timeB) return timeA - timeB;
    } else {
        if (ta->price != tb->price) return ta->price - tb->price;
    }
    
    return strcmp(trains[ta->trainIdx].trainID, trains[tb->trainIdx].trainID);
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
        
        if (line[pos] == '-' && pos + 1 < len && ((line[pos+1] >= 'a' && line[pos+1] <= 'z') || (line[pos+1] >= 'A' && line[pos+1] <= 'Z'))) {
            pos++;
            char key = line[pos++];
            while (pos < len && line[pos] == ' ') pos++;
            
            params[paramCount][0] = key;
            int valLen = 1;
            while (pos < len && line[pos] != ' ') {
                // Check if next character is a parameter marker
                if (line[pos] == '-' && pos + 1 < len && ((line[pos+1] >= 'a' && line[pos+1] <= 'z') || (line[pos+1] >= 'A' && line[pos+1] <= 'Z'))) {
                    break;
                }
                params[paramCount][valLen++] = line[pos++];
            }
            params[paramCount][valLen] = '\0';
            paramCount++;
        } else {
            pos++; // Skip unexpected characters
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
    
    // Parse stopover times first
    pos = 0;
    train.stopoverTimes[0] = 0;
    for (int j = 1; j < train.stationNum - 1; j++) {
        int time = 0;
        while (o[pos] && o[pos] != '|' && o[pos] != '_') {
            time = time * 10 + (o[pos++] - '0');
        }
        train.stopoverTimes[j] = time;
        if (o[pos] == '|' || o[pos] == '_') pos++;
    }
    train.stopoverTimes[train.stationNum - 1] = 0;
    
    // Parse travel times
    pos = 0;
    train.travelTimes[0] = 0;
    for (int j = 1; j < train.stationNum; j++) {
        int time = 0;
        while (t[pos] && t[pos] != '|') {
            time = time * 10 + (t[pos++] - '0');
        }
        train.travelTimes[j] = train.travelTimes[j-1] + time;
        if (j > 1) {
            train.travelTimes[j] += train.stopoverTimes[j-1];
        }
        if (t[pos] == '|') pos++;
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
            // Check if train is released to determine seat availability
            if (train.released) {
                SeatInfo* seatInfo = getSeatInfo(idx, date);
                cout << seatInfo->seats[j];
            } else {
                cout << train.seatNum;
            }
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
    
    int queryDate = dateToInt(d);
    bool sortByTime = (p == nullptr || strcmp(p, "time") == 0);
    
    TicketResult results[MAX_TRAINS];
    int resultCount = 0;
    
    // Find all trains that go from s to t
    for (int i = 0; i < trainCount; i++) {
        if (!trains[i].exists || !trains[i].released) continue;
        
        Train& train = trains[i];
        int fromIdx = findStation(train, s);
        int toIdx = findStation(train, t);
        
        if (fromIdx == -1 || toIdx == -1 || fromIdx >= toIdx) continue;
        
        // Calculate the departure date from starting station
        int leaveTime = train.startHour * 60 + train.startMin + train.travelTimes[fromIdx];
        if (fromIdx > 0) leaveTime += train.stopoverTimes[fromIdx];
        int daysFromStart = leaveTime / 1440;
        int startDate = queryDate - daysFromStart;
        
        // Check if this date is within sale range
        if (startDate < train.saleStart || startDate > train.saleEnd) continue;
        
        // Calculate times
        int arriveTime = train.startHour * 60 + train.startMin + train.travelTimes[toIdx];
        
        // Calculate price
        int price = train.prices[toIdx] - train.prices[fromIdx];
        
        // Get minimum seats
        SeatInfo* seatInfo = getSeatInfo(i, startDate);
        int minSeat = seatInfo->seats[fromIdx];
        for (int j = fromIdx + 1; j < toIdx; j++) {
            if (seatInfo->seats[j] < minSeat) {
                minSeat = seatInfo->seats[j];
            }
        }
        
        results[resultCount].trainIdx = i;
        results[resultCount].fromIdx = fromIdx;
        results[resultCount].toIdx = toIdx;
        results[resultCount].leaveTime = leaveTime;
        results[resultCount].arriveTime = arriveTime;
        results[resultCount].price = price;
        results[resultCount].seat = minSeat;
        results[resultCount].date = startDate;
        resultCount++;
    }
    
    // Simple bubble sort (since qsort doesn't work well with custom context)
    for (int i = 0; i < resultCount - 1; i++) {
        for (int j = 0; j < resultCount - i - 1; j++) {
            if (compareTickets(&results[j], &results[j+1], sortByTime) > 0) {
                TicketResult temp = results[j];
                results[j] = results[j+1];
                results[j+1] = temp;
            }
        }
    }
    
    cout << resultCount << "\n";
    for (int i = 0; i < resultCount; i++) {
        Train& train = trains[results[i].trainIdx];
        char leaveTimeBuf[20], arriveTimeBuf[20];
        timeToStr(results[i].leaveTime, results[i].date, leaveTimeBuf);
        timeToStr(results[i].arriveTime, results[i].date, arriveTimeBuf);
        
        cout << train.trainID << " " << train.stations[results[i].fromIdx] << " "
             << leaveTimeBuf << " -> " << train.stations[results[i].toIdx] << " "
             << arriveTimeBuf << " " << results[i].price << " " << results[i].seat << "\n";
    }
}

void cmd_query_transfer(char params[][105], int paramCount) {
    cout << "0\n";
}

void cmd_buy_ticket(char params[][105], int paramCount) {
    const char* u = getParam(params, paramCount, 'u');
    const char* i = getParam(params, paramCount, 'i');
    const char* d = getParam(params, paramCount, 'd');
    const char* n = getParam(params, paramCount, 'n');
    const char* f = getParam(params, paramCount, 'f');
    const char* t = getParam(params, paramCount, 't');
    const char* q = getParam(params, paramCount, 'q');
    
    int uIdx = findUser(u);
    if (uIdx == -1 || !loggedIn[uIdx]) {
        cout << "-1\n";
        return;
    }
    
    int trainIdx = findTrain(i);
    if (trainIdx == -1 || !trains[trainIdx].released) {
        cout << "-1\n";
        return;
    }
    
    Train& train = trains[trainIdx];
    int fromIdx = findStation(train, f);
    int toIdx = findStation(train, t);
    
    if (fromIdx == -1 || toIdx == -1 || fromIdx >= toIdx) {
        cout << "-1\n";
        return;
    }
    
    int num = atoi(n);
    if (num <= 0 || num > train.seatNum) {
        cout << "-1\n";
        return;
    }
    
    int queryDate = dateToInt(d);
    
    // Calculate the departure date from starting station
    int leaveTime = train.startHour * 60 + train.startMin + train.travelTimes[fromIdx];
    if (fromIdx > 0) leaveTime += train.stopoverTimes[fromIdx];
    int daysFromStart = leaveTime / 1440;
    int startDate = queryDate - daysFromStart;
    
    if (startDate < train.saleStart || startDate > train.saleEnd) {
        cout << "-1\n";
        return;
    }
    
    // Calculate price
    int price = train.prices[toIdx] - train.prices[fromIdx];
    int totalPrice = price * num;
    
    // Check seat availability
    SeatInfo* seatInfo = getSeatInfo(trainIdx, startDate);
    int minSeat = seatInfo->seats[fromIdx];
    for (int j = fromIdx + 1; j < toIdx; j++) {
        if (seatInfo->seats[j] < minSeat) {
            minSeat = seatInfo->seats[j];
        }
    }
    
    bool acceptQueue = (q != nullptr && strcmp(q, "true") == 0);
    
    if (minSeat >= num) {
        // Purchase successful
        for (int j = fromIdx; j < toIdx; j++) {
            seatInfo->seats[j] -= num;
        }
        
        Order& order = orders[orderCount++];
        order.orderId = orderCount - 1;
        strcpy(order.username, u);
        strcpy(order.trainID, i);
        order.date = queryDate;
        order.fromStation = fromIdx;
        order.toStation = toIdx;
        order.num = num;
        order.status = 0; // success
        order.price = totalPrice;
        order.timestamp = currentTime++;
        order.trainIdx = trainIdx;
        
        cout << totalPrice << "\n";
    } else if (acceptQueue) {
        // Add to queue
        Order& order = orders[orderCount++];
        order.orderId = orderCount - 1;
        strcpy(order.username, u);
        strcpy(order.trainID, i);
        order.date = queryDate;
        order.fromStation = fromIdx;
        order.toStation = toIdx;
        order.num = num;
        order.status = 1; // pending
        order.price = totalPrice;
        order.timestamp = currentTime++;
        order.trainIdx = trainIdx;
        
        cout << "queue\n";
    } else {
        cout << "-1\n";
    }
}

void cmd_query_order(char params[][105], int paramCount) {
    const char* u = getParam(params, paramCount, 'u');
    
    int uIdx = findUser(u);
    if (uIdx == -1 || !loggedIn[uIdx]) {
        cout << "-1\n";
        return;
    }
    
    // Count user's orders
    Order userOrders[MAX_ORDERS];
    int count = 0;
    for (int i = 0; i < orderCount; i++) {
        if (strcmp(orders[i].username, u) == 0) {
            userOrders[count++] = orders[i];
        }
    }
    
    // Sort by timestamp descending
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (userOrders[j].timestamp < userOrders[j+1].timestamp) {
                Order temp = userOrders[j];
                userOrders[j] = userOrders[j+1];
                userOrders[j+1] = temp;
            }
        }
    }
    
    cout << count << "\n";
    for (int i = 0; i < count; i++) {
        Order& order = userOrders[i];
        Train& train = trains[order.trainIdx];
        
        const char* statusStr = "success";
        if (order.status == 1) statusStr = "pending";
        else if (order.status == 2) statusStr = "refunded";
        
        // Calculate times
        int leaveTime = train.startHour * 60 + train.startMin + train.travelTimes[order.fromStation];
        if (order.fromStation > 0) leaveTime += train.stopoverTimes[order.fromStation];
        int daysFromStart = leaveTime / 1440;
        int startDate = order.date - daysFromStart;
        
        int arriveTime = train.startHour * 60 + train.startMin + train.travelTimes[order.toStation];
        
        char leaveTimeBuf[20], arriveTimeBuf[20];
        timeToStr(leaveTime, startDate, leaveTimeBuf);
        timeToStr(arriveTime, startDate, arriveTimeBuf);
        
        cout << "[" << statusStr << "] " << order.trainID << " "
             << train.stations[order.fromStation] << " " << leaveTimeBuf << " -> "
             << train.stations[order.toStation] << " " << arriveTimeBuf << " "
             << order.price << " " << order.num << "\n";
    }
}

void cmd_refund_ticket(char params[][105], int paramCount) {
    const char* u = getParam(params, paramCount, 'u');
    const char* n = getParam(params, paramCount, 'n');
    
    int uIdx = findUser(u);
    if (uIdx == -1 || !loggedIn[uIdx]) {
        cout << "-1\n";
        return;
    }
    
    int refundIdx = (n == nullptr) ? 1 : atoi(n);
    
    // Find user's orders sorted by timestamp
    Order* userOrders[MAX_ORDERS];
    int count = 0;
    for (int i = 0; i < orderCount; i++) {
        if (strcmp(orders[i].username, u) == 0) {
            userOrders[count++] = &orders[i];
        }
    }
    
    // Sort by timestamp descending
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (userOrders[j]->timestamp < userOrders[j+1]->timestamp) {
                Order* temp = userOrders[j];
                userOrders[j] = userOrders[j+1];
                userOrders[j+1] = temp;
            }
        }
    }
    
    if (refundIdx < 1 || refundIdx > count) {
        cout << "-1\n";
        return;
    }
    
    Order* order = userOrders[refundIdx - 1];
    if (order->status == 2) { // already refunded
        cout << "-1\n";
        return;
    }
    
    if (order->status == 0) { // success, need to return seats
        Train& train = trains[order->trainIdx];
        int leaveTime = train.startHour * 60 + train.startMin + train.travelTimes[order->fromStation];
        if (order->fromStation > 0) leaveTime += train.stopoverTimes[order->fromStation];
        int daysFromStart = leaveTime / 1440;
        int startDate = order->date - daysFromStart;
        
        SeatInfo* seatInfo = getSeatInfo(order->trainIdx, startDate);
        for (int j = order->fromStation; j < order->toStation; j++) {
            seatInfo->seats[j] += order->num;
        }
        
        // Try to satisfy pending orders
        for (int i = 0; i < orderCount; i++) {
            if (orders[i].status == 1 && orders[i].trainIdx == order->trainIdx) {
                Order& pendingOrder = orders[i];
                Train& pTrain = trains[pendingOrder.trainIdx];
                int pLeaveTime = pTrain.startHour * 60 + pTrain.startMin + pTrain.travelTimes[pendingOrder.fromStation];
                if (pendingOrder.fromStation > 0) pLeaveTime += pTrain.stopoverTimes[pendingOrder.fromStation];
                int pDaysFromStart = pLeaveTime / 1440;
                int pStartDate = pendingOrder.date - pDaysFromStart;
                
                if (pStartDate == startDate) {
                    // Check if can satisfy
                    int minSeat = seatInfo->seats[pendingOrder.fromStation];
                    for (int j = pendingOrder.fromStation + 1; j < pendingOrder.toStation; j++) {
                        if (seatInfo->seats[j] < minSeat) {
                            minSeat = seatInfo->seats[j];
                        }
                    }
                    
                    if (minSeat >= pendingOrder.num) {
                        // Satisfy this order
                        for (int j = pendingOrder.fromStation; j < pendingOrder.toStation; j++) {
                            seatInfo->seats[j] -= pendingOrder.num;
                        }
                        pendingOrder.status = 0; // success
                    }
                }
            }
        }
    }
    
    order->status = 2; // refunded
    cout << "0\n";
}

void cmd_clean() {
    userCount = 0;
    trainCount = 0;
    orderCount = 0;
    seatDataCount = 0;
    currentTime = 0;
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
