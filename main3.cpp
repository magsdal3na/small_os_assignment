#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <algorithm>
#include <iomanip>
#include <cmath>
#include <string>

using namespace std;

//Represents a process definition from input
struct Process {
    int id;
    int arrival_time;
    int relative_deadline;
    int period;
    int execution_time; //Service time (C_i)
};

//Types of events in the simulation
enum EventType {
    PROCESS_ARRIVAL,
    PROCESS_COMPLETION
};

//Represents a process instance (job)
struct Job {
    int process_id;
    int job_id; //Instance counter
    int release_time;
    int absolute_deadline;
    int period;
    int remaining_time;
    int execution_time;
};

//Event struct for the priority queue
struct Event {
    int time;
    EventType type;
    Job job;

    //Primary sort by event time (earliest event first)
    bool operator>(const Event& other) const {
        if(time != other.time)
            return time > other.time;
        //If times match, completions precede arrivals to free CPU
        return type > other.type;
    }
};

enum Algorithm { RM, DM, EDF };

//Function to calculate Greatest Common Divisor (GCD)
long long gcd(long long a, long long b) {
    while(b) {
        a %= b;
        swap(a, b);
    }
    return a;
}

//Function to calculate Least Common Multiple (LCM)
long long lcm(long long a, long long b) {
    if(a == 0 || b == 0) {
        return 0;
    }
    return (a / gcd(a, b)) * b;
}

//Function to calculate the simulation end time
int calculate_hyperperiod(const vector<Process>& processes) {
    long long hyperperiod = processes[0].period;
    for(size_t i = 1; i < processes.size(); ++i) {
        hyperperiod = lcm(hyperperiod, processes[i].period);
    }
    return static_cast<int>(hyperperiod);
}

//Priority commparison helper for ready queue depending on the policy
bool is_higher_priority(const Job& a, const Job& b, Algorithm algo, const vector<Process>& procs) {
    if(algo == RM) {
        //Rate Monotonic: shorter period = higher priority
        if(a.period != b.period) {
            return a.period < b.period;
        }
    }
    else if(algo == DM) {
            //Deadline Monotonic: shorter relative deadline = higher priority
            int rel_a = procs[a.process_id - 1].relative_deadline;
            int rel_b = procs[b.process_id - 1].relative_deadline;
            if(rel_a != rel_b) {
                return rel_a < rel_b;
            }
    }
    else if(algo == EDF) {
                //Earliest Deadline First: dynamic, earlier absolute deadline = higher priority
                if(a.absolute_deadline != b.absolute_deadline) {
                    return a.absolute_deadline < b.absolute_deadline;
                }
            }
    //Tie-breaker: lower process ID
    return a.process_id < b.process_id;
}

//Stores finish times for output reporting
struct JobFinishRecord {
    int process_id;
    int job_id;
    int finish_time;
};

//Simulation Runner
void run_simulation(Algorithm algo, const vector<Process>& processes, int context_switch_overhead) {
    string algo_name;
    switch(algo) {
        case RM: algo_name = "Rate Monotonic (RM)"; break;
        case DM: algo_name = "Deadline Monotonic (DM)"; break;
        case EDF: algo_name = "Earliest Deadline First (EDF)"; break;
    }

    cout << "\n======================================================\n";
    cout << " Running " << algo_name << " Simulation\n";
    cout << "\n======================================================\n";

    int hyperperiod = calculate_hyperperiod(processes);
    //Limit hyperperiod display to avoid excessively long runs in output if period LCM is massive
    int max_sim_time = min(hyperperiod, 500);

    priority_queue<Event, vector<Event>, greater<Event>> event_queue;
    vector<Job> ready_queue;

    //Schedule initial arrivals
    for(const auto& p : processes) {
        Job first_job = { p.id, 1, p.arrival_time, p.arrival_time + p.relative_deadline, p.period, p.execution_time, p.execution_time };
        event_queue.push({ p.arrival_time, PROCESS_ARRIVAL, first_job });
    }

    int current_time = 0;
    bool cpu_busy = false;
    Job current_job;
    int current_job_start_time = 0;

    int total_cpu_busy_time = 0;
    bool feasible = true;
    int failure_time = -1;
    int failed_process_id = -1;

    vector<JobFinishRecord> finish_records;

    while(!event_queue.empty()) {
        Event current_event = event_queue.top();

        if(current_event.time > max_sim_time) {
            break;
        }
        
        event_queue.pop();

        int event_time = current_event.time;

        //Advance simulation clock and accumulate CPU busy time if running a job
        if(cpu_busy && event_time > current_time) {
            int elapsed = event_time - current_time;
            current_job.remaining_time -= elapsed;
            total_cpu_busy_time += elapsed;
        }

        current_time = event_time;

        if(current_event.type == PROCESS_ARRIVAL) {
            Job new_job = current_event.job;

            //Check deadline miss on arrival (if previous instance did not finish before new arrival)
            for(const auto& r_job : ready_queue) {
                if(r_job.process_id == new_job.process_id && feasible) {
                        feasible = false;
                        failure_time = current_time;
                        failed_process_id = new_job.process_id;
                    }
                }

            ready_queue.push_back(new_job);

            //Schedule future periodic arrival for this process
            if(current_time + new_job.period <= max_sim_time) {
                const auto& p = processes[new_job.process_id - 1];
                Job next_job = {
                    p.id,
                    new_job.job_id + 1,
                    current_time + p.period,
                    current_time + p.period + p.relative_deadline,
                    p.period,
                    p.execution_time,
                    p.execution_time
                };
                event_queue.push({ next_job.release_time, PROCESS_ARRIVAL, next_job });
            }
        }

        //Preemption check if CPU is busy
        if(current_event.type == PROCESS_ARRIVAL && cpu_busy) {

            Job new_job = current_event.job;
            //If newly arrived job has higher priority than currently running job
            if(is_higher_priority(new_job, current_job, algo, processes)) {
                //Put current job back in ready queue
                ready_queue.push_back(current_job);

                cpu_busy = false;

                //Remove stale completion event for current_job
                vector<Event> temp_events;

                while(!event_queue.empty()) {
                    Event ev = event_queue.top();
                    event_queue.pop();
                    
                    if(!(ev.type == PROCESS_COMPLETION && ev.job.process_id == current_job.process_id && ev.job.job_id == current_job.job_id)) {
                        temp_events.push_back(ev);
                    }
                }
                for(const auto& ev: temp_events) {
                    event_queue.push(ev);
                }
            }
        }

        else if(current_event.type == PROCESS_COMPLETION) {

            cpu_busy = false;

            finish_records.push_back({ current_event.job.process_id, current_event.job.job_id, current_time });

            //Check if deadline was missed
            if(current_time > current_event.job.absolute_deadline && feasible) {
                feasible = false;
                failure_time = current_time;
                failed_process_id = current_event.job.process_id;
            }
        }

        //Check preemption or scheduling next job
        if(!cpu_busy && !ready_queue.empty()) {
            //Sort ready queue based on scheduling algorithm policy
            sort(ready_queue.begin(), ready_queue.end(), [&](const Job& a, const Job& b) {
                return is_higher_priority(a, b, algo, processes);
            });

            current_job = ready_queue.front();
            ready_queue.erase(ready_queue.begin());

            cpu_busy = true;
            current_job_start_time = current_time;
            
            event_queue.push({ current_time + current_job.remaining_time, PROCESS_COMPLETION, current_job });
        }
    }

    //---Output Results ---
    cout << algo_name << " Results:\n";
    if(feasible) {
        cout << "Schedule feasible from 0 to " << max_sim_time << " units.\n";
    }
    else {
        cout << "Schedule feasible from 0 to " << failure_time << " units. At "
             << failure_time << ", Process " << failed_process_id << " did not meet deadline.\n";
    }

    cout << "CPU time took " << total_cpu_busy_time << " units out of " << max_sim_time << " total units.\n";
    cout << "CPU Utilization: " << fixed << setprecision(2)
         << (static_cast<double>(total_cpu_busy_time) / max_sim_time) * 100.0 << "%\n\n";

    //Summary per process finish times
    for(const auto& p : processes) {
        cout << "Process " << p.id << ": Arrival Time: " << p.arrival_time
             << ", Service: " << p.execution_time << ", Period: " << p.period << ", Finishes at: [";
             bool first = true; 
             for(const auto& rec : finish_records) {
                if(rec.process_id == p.id) {
                    if(!first) {
                        cout << ", ";
                    }
                        cout << rec.finish_time;
                        first = false;
                }
             }
             cout << "]\n";
    }
}

int main() {

    string filename;

    cout << "There are three scenario files for this program:" << endl;
    cout << "scenario1.txt, scenario2.txt, scenario3.txt" << endl;
    cout << "Please enter the name of the file to use below: " << endl;

    cin >> filename;

    ifstream infile(filename);

    if(!infile) {
        cerr << "error opening input file.\n";
        return 1;
    }

    int num_processes, process_switch_overhead;
    infile >> num_processes >> process_switch_overhead;

    //Assigns execution/service times according to assignment recommendations:
    //Service times chosen to keep total CPU load near feasible boundaries
    //Pulls service times from .txt file
    vector<int> service_times(num_processes);

    for(int i = 0; i < num_processes; ++i) {
        infile >> service_times[i];
    }

    vector<Process> processes(num_processes);

    for(int i = 0; i < num_processes; ++i) {
        infile >> processes[i].id
               >> processes[i].arrival_time
               >> processes[i].relative_deadline
               >> processes[i].period;

        processes[i].execution_time = service_times[i];
    }
    infile.close();

    //Run simulation for RM, DM, and EDF 
    run_simulation(RM, processes, process_switch_overhead);
    run_simulation(DM, processes, process_switch_overhead);
    run_simulation(EDF, processes, process_switch_overhead);

    return 0;
}

