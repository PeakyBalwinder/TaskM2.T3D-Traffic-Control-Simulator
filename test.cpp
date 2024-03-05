#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <ctime>
#include <random>
#include <algorithm>
#include <map>

using namespace std;

const int NUM_OF_TRAFFIC_LIGHTS = 5;
const int BUFFER_CAPACITY = 10;
const int NUM_OF_MEASUREMENTS = 12;
const int TOP_CONGESTED = 3;

mutex buffer_lock;
queue<tuple<string, int, int>> traffic_buffer;
map<int, int> congested_traffic_lights;

ofstream dataFile; // File to write data

// Producer function
void generate_traffic_data() {
    while (true) {
        // Simulate traffic signal data generation
        auto now = chrono::system_clock::now();
        auto now_time = chrono::system_clock::to_time_t(now);
        string timestamp = ctime(&now_time);
        int traffic_light_id = rand() % NUM_OF_TRAFFIC_LIGHTS + 1;
        int cars_passed = rand() % 50 + 1;

        // Add data to buffer
        {
            lock_guard<mutex> guard(buffer_lock);
            if (traffic_buffer.size() >= BUFFER_CAPACITY) {
                cout << "Buffer full, waiting to add data..." << endl;
            }
            traffic_buffer.push(make_tuple(timestamp, traffic_light_id, cars_passed));
            cout << "Generated: " << timestamp << ", Traffic Light ID: " << traffic_light_id << ", Cars Passed: " << cars_passed << endl;

            // Write data to file
            dataFile << "Generated: " << timestamp << ", Traffic Light ID: " << traffic_light_id << ", Cars Passed: " << cars_passed << endl;
        }
        this_thread::sleep_for(chrono::seconds(3));
    }
}

// Consumer function
void process_traffic_data() {
    while (true) {
        this_thread::sleep_for(chrono::seconds(10));

        // Process traffic data
        {
            lock_guard<mutex> guard(buffer_lock);
            while (!traffic_buffer.empty()) {
                auto data = traffic_buffer.front();
                traffic_buffer.pop();
                string timestamp = get<0>(data);
                int traffic_light_id = get<1>(data);
                int cars_passed = get<2>(data);
                if (congested_traffic_lights.find(traffic_light_id) != congested_traffic_lights.end()) {
                    congested_traffic_lights[traffic_light_id] += cars_passed;
                } else {
                    congested_traffic_lights[traffic_light_id] = cars_passed;
                }
            }
        }

        // Find top N congested traffic lights
        vector<pair<int, int>> sorted_congested(congested_traffic_lights.begin(), congested_traffic_lights.end());
        partial_sort(sorted_congested.begin(), sorted_congested.begin() + min(TOP_CONGESTED, (int)sorted_congested.size()), sorted_congested.end(),
                     [](const pair<int, int> &a, const pair<int, int> &b) { return a.second > b.second; });
        cout << "Top " << TOP_CONGESTED << " congested traffic lights at " << chrono::system_clock::to_time_t(chrono::system_clock::now()) << " - ";
        for (int i = 0; i < min(TOP_CONGESTED, (int)sorted_congested.size()); ++i) {
            cout << sorted_congested[i].first << " (Cars Passed: " << sorted_congested[i].second << "), ";
        }
        cout << endl;

        // Write data to file
        dataFile << "Top " << TOP_CONGESTED << " congested traffic lights at " << chrono::system_clock::to_time_t(chrono::system_clock::now()) << " - ";
        for (int i = 0; i < min(TOP_CONGESTED, (int)sorted_congested.size()); ++i) {
            dataFile << sorted_congested[i].first << " (Cars Passed: " << sorted_congested[i].second << "), ";
        }
        dataFile << endl;
    }
}

int main() {
    srand(time(nullptr));

    // Open file for writing
    dataFile.open("traffic_data.txt");

    // Create producer and consumer threads
    thread producer_thread(generate_traffic_data);
    thread consumer_thread(process_traffic_data);

    // Join threads
    producer_thread.join();
    consumer_thread.join();

    // Close file
    dataFile.close();

    return 0;
}
