#include "RoundRobin.h"

/*
This is a constructor for RoundRobin Scheduler, you should use the extractProcessInfo function first
to load process information to process_info and then sort process by arrival time;

Also initialize time_quantum
*/
RoundRobin::RoundRobin(string file, int time_quantum) : time_quantum(time_quantum) {
	extractProcessInfo(file);

	sort(processVec.begin(), processVec.end(), [](const Process& a, const Process& b) {
		return a.get_arrival_time() < b.get_arrival_time();
	});
}

// Schedule tasks based on RoundRobin Rule
// the jobs are put in the order the arrived
// Make sure you print out the information like we put in the document

void RoundRobin::schedule_tasks() {
    int currentTime = 0;
    
    while (!processVec.empty() || !processQ.empty()) {
        for (auto it = processVec.begin(); it != processVec.end();) {
            if (it->get_arrival_time() <= currentTime) {
                processQ.push(*it);
                it = processVec.erase(it);
            } else {
                ++it;
            }
        }

        if (processQ.empty()) {
            print(currentTime, -1, false);
            currentTime++;
        } else {
            Process currentProcess = processQ.front();
            processQ.pop();

            int remainingTime = std::min(time_quantum, currentProcess.get_remaining_time());

            for (int t = 0; t < remainingTime; t++) {
                print(currentTime, currentProcess.getPid(), false);
                currentTime++;
            }

            currentProcess.Run(remainingTime);

            if (currentProcess.is_Completed()) {
                print(currentTime, currentProcess.getPid(), true);
            } else {
                for (auto it = processVec.begin(); it != processVec.end();) {
                    if (it->get_arrival_time() <= currentTime) {
                        processQ.push(*it);
                        it = processVec.erase(it);
                    } else {
                        ++it;
                    }
                }
                processQ.push(currentProcess);
            }
        }
    }
}


/*************************** 
ALL FUNCTIONS UNDER THIS LINE ARE COMPLETED FOR YOU
You can modify them if you'd like, though :)
***************************/


// Default constructor
RoundRobin::RoundRobin() {
	time_quantum = 0;
}

// Time quantum setter
void RoundRobin::set_time_quantum(int quantum) {
	this->time_quantum = quantum;
}

// Time quantum getter
int RoundRobin::get_time_quantum() {
	return time_quantum;
}

// Print function for outputting system time as part of the schedule tasks function
void RoundRobin::print(int system_time, int pid, bool isComplete){
	string s_pid = pid == -1 ? "NOP" : to_string(pid);
	cout << "System Time [" << system_time << "].........Process[PID=" << s_pid << "] ";
	if (isComplete)
		cout << "finished its job!" << endl;
	else
		cout << "is Running" << endl;
}

// Read a process file to extract process information
// All content goes to proces_info vector
void RoundRobin::extractProcessInfo(string file){
	// open file
	ifstream processFile (file);
	if (!processFile.is_open()) {
		perror("could not open file");
		exit(1);
	}

	// read contents and populate process_info vector
	string curr_line, temp_num;
	int curr_pid, curr_arrival_time, curr_burst_time;
	while (getline(processFile, curr_line)) {
		// use string stream to seperate by comma
		stringstream ss(curr_line);
		getline(ss, temp_num, ',');
		curr_pid = stoi(temp_num);
		getline(ss, temp_num, ',');
		curr_arrival_time = stoi(temp_num);
		getline(ss, temp_num, ',');
		curr_burst_time = stoi(temp_num);
		Process p(curr_pid, curr_arrival_time, curr_burst_time);

		processVec.push_back(p);
	}

	// close file
	processFile.close();
}
