#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <queue>
#include <algorithm>
#include <sstream>

using namespace std;
struct Task {
    string id;
    int burst_time;
    int remaining_time;
    vector<string> memory_blocks;
};

class CacheLevel {
private:
    int capacity;
    int latency;
    list<string> blocks; 

public:
    CacheLevel(int cap, int lat) : capacity(cap), latency(lat) {}

    bool contains(const string& block) {
        return find(blocks.begin(), blocks.end(), block) != blocks.end();
    }

    void remove(const string& block) {
        auto it = find(blocks.begin(), blocks.end(), block);
        if (it != blocks.end()) {
            blocks.erase(it);
        }
    }
    string insert(const string& block) {
        if (contains(block)) return "";

        string evicted = "";
        if (blocks.size() >= capacity) {
            evicted = blocks.front();
            blocks.pop_front();
        }
        blocks.push_back(block);
        return evicted;
    }

    int getLatency() const { return latency; }

    void printState(const string& levelName) {
        cout << "  " << levelName << ": [";
        for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
            cout << *it;
            if (next(it) != blocks.rend()) cout << ", ";
        }
        cout << "]";
    }
};

class MemoryHierarchy {
private:
    CacheLevel L1;
    CacheLevel L2;
    CacheLevel L3;
    int ram_latency;
    int total_ram_accesses;
    int total_cycles;

    void promoteToL1(const string& block) {
        string evicted_l1 = L1.insert(block);
        
        if (!evicted_l1.empty()) {
            cout << "  Promoting " << block << " -> L1 (" << evicted_l1 << " evicted)\n";
            string evicted_l2 = L2.insert(evicted_l1);
            
            if (!evicted_l2.empty()) {
                L3.insert(evicted_l2);
            }
        } else {
            cout << "  Promoting " << block << " -> L1\n";
        }
    }

public:
    MemoryHierarchy() : 
        L1(32, 4), 
        L2(128, 12), 
        L3(512, 40), 
        ram_latency(200), 
        total_ram_accesses(0), 
        total_cycles(0) {}

    int requestMemory(const string& block) {
        int cycles_spent = 0;

        if (L1.contains(block)) {
            cout << " -> HIT (L1)\n";
            cycles_spent += L1.getLatency();
        } 
        else if (L2.contains(block)) {
            cout << " >> MISS L1, HIT L2\n";
            cycles_spent += L2.getLatency();
            L2.remove(block); 
            promoteToL1(block);
        } 
        else if (L3.contains(block)) {
            cout << " >> MISS L1/L2, HIT L3\n";
            cycles_spent += L3.getLatency();
            L3.remove(block); 
            promoteToL1(block);
        } 
        else {
            cout << " >> MISS at all levels. Fetching from RAM...\n";
            total_ram_accesses++;
            cycles_spent += ram_latency;
            promoteToL1(block);
        }
        
        total_cycles += cycles_spent;
        return cycles_spent;
    }

    void printCaches() {
        L1.printState("L1"); cout << "\n";
        L2.printState("L2"); cout << "\n";
        L3.printState("L3"); cout << "\n";
    }
    
    int getRamAccesses() const { return total_ram_accesses; }
    int getTotalCycles() const { return total_cycles; }
};

class CPUScheduler {
private:
    queue<Task> ready_queue;
    MemoryHierarchy mem;
    int current_cycle;
    int completed_tasks;

public:
    CPUScheduler() : current_cycle(1), completed_tasks(0) {}

    void addTask(const Task& t) {
        ready_queue.push(t);
    }

    void runRoundRobin(int quantum) {
        while (!ready_queue.empty()) {
            Task current_task = ready_queue.front();
            ready_queue.pop();

            int time_to_run = min(quantum, current_task.remaining_time);

            for (int i = 0; i < time_to_run; ++i) {
                cout << "Cycle " << current_cycle << " - Running: " << current_task.id << "\n";
                
                if (!current_task.memory_blocks.empty()) {
                    int block_index = (current_task.burst_time - current_task.remaining_time) % current_task.memory_blocks.size();
                    string block_needed = current_task.memory_blocks[block_index];
                    
                    cout << "  Requesting: " << block_needed;
                    mem.requestMemory(block_needed);
                    mem.printCaches();
                }

                current_task.remaining_time--;
                current_cycle++;
                cout << "--------------------------------------\n";
            }

            if (current_task.remaining_time > 0) {
                ready_queue.push(current_task);
            } else {
                completed_tasks++;
            }
        }
        printFinalResults(quantum);
    }

    void printFinalResults(int quantum) {
        cout << "\n=== Final Results ===\n";
        cout << "Total Cycles:    " << (current_cycle - 1) + mem.getTotalCycles() << "\n";
        cout << "Tasks Completed: " << completed_tasks << "\n";
        cout << "Scheduler:       Round Robin (quantum = " << quantum << ")\n";
        cout << "RAM Accesses:    " << mem.getRamAccesses() << "\n";
    }
};

int main() {
    CPUScheduler scheduler;
    string line;
    while (getline(cin, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string word;
        Task t;

        ss >> word; 
        ss >> t.id; 
        
        ss >> word; 
        ss >> t.burst_time;
        t.remaining_time = t.burst_time;

        ss >> word; 
        while (ss >> word) {
            t.memory_blocks.push_back(word);
        }

        scheduler.addTask(t);
    }

    scheduler.runRoundRobin(3);

    return 0;
}