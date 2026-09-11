#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

using namespace std;

//Job struct to set up variables from jobs.txt
struct Job {
    int id;
    int startTime;
    int size;
    int interval;
    string endState;
};

//Function to print the memory
void printMemory(const vector<int>& memory) {
    cout << "Memory: ";
    
    //Printing each page and whether or not it's empty or has job
    for (int page : memory) {
        if(page == 0) {
            cout << "[ ]";
        }
        else {
            cout << "[" << page << "]";
        }
    }
    cout << endl;
}

//Function to find First-Fit
int findFirstFit(const vector<int>& memory, int requiredSize) {
    
    //Initialize int variable that shows free slots
    int freeCount = 0;

    //Checking to see if enough memory is available for the required size
    for (int i = 0; i < memory.size(); i++) {
        
        if (memory[i] == 0) {
            freeCount++;

            if (freeCount == requiredSize) {
                return i - requiredSize + 1; //Captures correct page start
            }
        }

        else {
            freeCount = 0;
        }
    }

    return -1;
}

//Function for deallocating a job
void freeJob(vector<int>& memory, int jobID) {
    for(int i = 0; i <  memory.size(); i++) {
        if(memory[i] == jobID) {
            memory[i] = 0;
        }
    }

    cout << "Freeing job " << jobID << " complete." << endl;
}

//Function to find Best-Fit
int findBestFit(const vector<int>& memory, int requiredSize) {

    //Initialize int variable that shows free slots
    int freeCount = 0;
    int bestSize = 0;
    int bestStart = -1;

    //Checking to see if enough memory is available for the required size
    for (int i = 0; i < memory.size(); i++) {
        
        if (memory[i] == 0) {
            freeCount++;
        }

        //Checking if the fit is the best size and start
        else {
            if(freeCount >= requiredSize && (bestSize == 0 || freeCount < bestSize)) {
                bestSize = freeCount;
                bestStart = i - freeCount;
            }

            freeCount = 0;
        }
    }

    //Checks for free space at the end of memory
    if(freeCount >= requiredSize && (bestSize == 0 || freeCount < bestSize)) {
        bestSize = freeCount;
        bestStart = memory.size() - freeCount;
    }

    return bestStart; 
}

//Function to find Worst-Fit
int findWorstFit(const vector<int>& memory, int requiredSize) {

    //Initializing variables for use in the function
    int freeCount = 0;
    int worstSize = 0;
    int worstStart = -1;

    //For loop to check the size of memory
    for(int i = 0; i < memory.size(); i++) {

        //If memory is empty, increase freeCount
        if(memory[i] == 0) {
            freeCount++;
        }

        //If the memory isn't empty, check for fit
        else {
            if(freeCount >= requiredSize && freeCount > worstSize) {
                worstSize = freeCount;
                worstStart = i - freeCount;
            }

            freeCount = 0;
        }
    }

    //Check free space at the end of memory
    if(freeCount >= requiredSize && freeCount > worstSize) {

        worstSize = freeCount;
        worstStart = memory.size() - freeCount;
    }

    return worstStart;
}

//Struct to represent a completion event to help 
//calculate completionTime
struct Event {
    int time;
    int jobId;
};

//Function to allocate a job
bool allocateJob(vector<int>& memory, const Job& job, int location) {

    if(location == -1) {
        return false;
    }

    for(int i = location; i < location + job.size; i++) {
        memory[i] = job.id;
    }

    return true;
}

//Function to choose the allocation strategy
int findLocation(const vector<int>& memory, int requiredSize, string strategy) {

    //Returns if the strategy is "First-Fit"
    if(strategy == "First-Fit") {
        return findFirstFit(memory, requiredSize);
    }

    //Returns if strategy is "Best-Fit"
    if(strategy == "Best-Fit") {
        return findBestFit(memory, requiredSize);
    }

    //Returns if strategy is "Worst-Fit"
    if(strategy == "Worst-Fit") {
        return findWorstFit(memory, requiredSize);
    }

    return -1;
}

//Simulation function to replace original loops in main
//This will make it easier to run repeated simulations without
//bogging down the program
void simulate(const vector<Job>& jobs, string strategy) {

    //Setting up vector for memory
    vector<int> memory(20,0);

    //Output text
    cout << "\n====================================\n";
    cout << strategy << " Simulation\n";
    cout << "\n====================================\n";

    //Keep track of jobs currently in memory
    vector<Event> events;

    //Go through jobs in the order they appear in jobs.txt
    for(const Job& job : jobs) {

        int currentTime = job.startTime;

        //Output text with variables to print
        cout << "\nTime " << job.startTime << endl;

        //Frees jobs whose intervals have ended
        for(int i = 0; i < events.size(); i++) {
            if(events[i].time <= currentTime) {
                cout << "Job " << events[i].jobId
                     << " interval ended." << endl;

                     freeJob(memory, events[i].jobId);

                     //Remove this event
                     events.erase(events.begin() + i);
                     i--;
            }
        }

        cout << "Job " << job.id << " arrives." << endl;

        //Setting up location int to work with findLocation function
        int location = findLocation(memory, job.size, strategy);

        if(location == -1) {
            cout << "Not enough contiguous memory for Job "
                 << job.id << "." << endl;
        }

        else {
            allocateJob(memory, job, location);

            cout << "Job " << job.id
                 << " allocated starting at page "
                 << location << endl;

            //Calculate when the job finishes
            Event event;

            event.time = job.startTime + job.interval;
            event.jobId = job.id;

            events.push_back(event);
        }

        printMemory(memory);

    }
}

int main() {

    //Vector for jobs
    vector<Job> jobs;

    //Initializing variables (empty)
    int id;
    int startTime;
    int size;
    int interval;
    string endState;

    //Starting text to show program is booted/working
    cout << "Memory simulation starting..." << endl;

    //Setting up the input stream
    ifstream inputFile("jobs.txt");

    //Checking if the file opened correctly
    if (!inputFile) {
        cout << "Could not open jobs.txt" << endl;
        return 1; //Returns an error from not opening
    }

    //Reading the file
    while (inputFile >> id >> startTime >> size >> interval >> endState) {

        //Setting up input collector for each job
        Job job;

        job.id = id;
        job.startTime = startTime;
        job.size = size;
        job.interval = interval;
        job.endState = endState;

        //Adding jobs
        jobs.push_back(job);
    }

    sort(jobs.begin(), jobs.end(), [](const Job& a, const Job& b) {
        return a.startTime < b.startTime;
    });

    inputFile.close();

    //Printed output
    cout << "Jobs loaded: " << jobs.size() << endl;

    //Runs each allocation strategy based on
    //functions created above
    simulate(jobs, "First-Fit");
    simulate(jobs, "Best-Fit");
    simulate(jobs, "Worst-Fit");

    return 0;
}
