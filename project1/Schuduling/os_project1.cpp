/*  project1_submitty.cpp */
/* Created by Fangyan Xu & Chengjian Zheng & Feifei Pan on 10/9/16. */

/* Operating System Project 1 */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

class Process
{
public:
    void readInput(string line);
    int mTurnaroundTime, mWaitTime; // Time that we need to measure
    int remainingTime; // The count down (remaining time) for running or blocking
    
    // We need to intialize this upon every start of a running
    // or blocking process
    char proc_id;
    int arrivalTime, cpuBurstTime, numBurst, ioTime;
    int initialnumBurst;
};

void Process::readInput(string line)  // this method initializes the processes for simulation
{
    // initialize the state of all processe
    // Set measures to 0
    istringstream iss(line);
    string tmp;
    vector<string> specs;
    while (getline(iss, tmp, '|'))
        specs.push_back(tmp);
    
    proc_id = specs[0].c_str()[0];
    arrivalTime = atoi(specs[1].c_str());
    cpuBurstTime = atoi(specs[2].c_str());
    numBurst = atoi(specs[3].c_str());
    initialnumBurst = numBurst;
    ioTime = atoi(specs[4].c_str());
    
    mTurnaroundTime = 0;
    mWaitTime = 0;
    remainingTime = 0;
}


/* Compare functions for sorting in the list or vector */

bool compArrivalTime(const Process& p1,const Process& p2)
{
    if (p1.arrivalTime < p2.arrivalTime) {
        return true;
    }
    else if(p1.arrivalTime == p2.arrivalTime){
        if (p1.proc_id < p2.proc_id) {
            return true;
        }
    }
    return false;
}

bool compProcID(const Process& p1, const Process& p2)
{
    if (p1.proc_id < p2.proc_id)
        return true;
    else
        return false;
}

bool operator==(const Process& p1, const Process& p2)
{
    if (p1.proc_id==p2.proc_id)
        return true;
    else
        return false;
}

bool compRemainingTime(const Process& p1,const Process& p2)
{
    if (p1.remainingTime < p2.remainingTime) {
        return true;
    }
    else if(p1.remainingTime == p2.remainingTime){
        if (p1.proc_id < p2.proc_id) {
            return true;
        }
    }
    return false;
}

/* The class CPU contains the simulator. It's variables are the states of the CPU */
class CPU
{
public:
    CPU(vector<Process>& all_input_processes);
    const char* strReadyQ();
    void intoIOVec(Process&);
    void intoReadyQ(Process&);
    void updateInitialProcessVec();
    void updateIOVec();
    void updateReadyQ();
    
    // all of these three methods can be simulated by RR
    void simulate_RR(int t_cs, int t_slice, string method, const char* filename);
    
    // We use four containers (vector/lists) for grouping the processes according to their states
    // 1. initialProcessesVec : the processes as initialized (haven't arrived yet)
    // 2. readyQ : Processes that have arrived in the current time step
    // 3. ioVec : Processes that finish some cpu Burst and are blocked on I/O
    // 4. finishedVec : Processes that have finished all of their cpuBursts
    
    vector<Process> initialProcessesVec; /* all processes are initially stored here*/
    list<Process> readyQ;
    list<Process> ioVec;
    list<Process> finishedVec;
    Process cpuProcess;
    string state;  /* The state of the CPU, can be RUNNING, SWITCH_IN, SWITCH_OUT, SWITCH_FULL or IDLE */
    int cpuTimer;
    int numContextSwitch, numPreemption;
    string qMethod;
    int simu_time;
};

// the constructor of CPU
CPU::CPU(vector<Process>& all_input_processes)
{
    initialProcessesVec = all_input_processes;
}

// Necessary operations for pushing a process into readyQ (FCFS or SJF)
void CPU::intoReadyQ(Process& proc)
{
    readyQ.push_back(proc);
    if (qMethod == "SJF")
        readyQ.sort(compRemainingTime);
    return;
}

// Necessary operations for pushig a process into ioVec
void CPU::intoIOVec(Process& proc)
{
    proc.arrivalTime = simu_time + proc.ioTime;
    proc.numBurst--;
    ioVec.push_back(proc);
    printf("time %dms: Process %c completed a CPU burst; %d to go %s\n",
           simu_time, proc.proc_id, proc.numBurst, strReadyQ());
    printf("time %dms: Process %c blocked on I/O until time %dms %s\n",
           simu_time, proc.proc_id, proc.arrivalTime, strReadyQ());
}

// Generate a c string for printing out the readyQ
const char* CPU::strReadyQ()
{
    string str = "[Q ";
    
    if (readyQ.empty())
    {
        str = "[Q empty]";
        return str.c_str();
    }
    
    for (list<Process>::iterator it = readyQ.begin(); it != readyQ.end(); ++it)
    {
        str += it->proc_id;
        str += ' ';
    }
    str[str.length()-1]=']';
    return str.c_str();
}

// In every iteration, we first update initialProcessVec to push arrived processes into readyQ
void CPU::updateInitialProcessVec()
{
    list<Process> arrivalProcess;
    vector<Process>::iterator it = initialProcessesVec.begin();
    
    while (it != initialProcessesVec.end())
    {
        if (it->arrivalTime <= simu_time)
        {
            arrivalProcess.push_back(*it);
            initialProcessesVec.erase(it);
        }
        else
            ++it;
    }
    arrivalProcess.sort(compArrivalTime);
    for (list<Process>::iterator it = arrivalProcess.begin(); it != arrivalProcess.end(); ++it)
    {
        // when some process arrives, we initialize the measures.
        it->mWaitTime = 0;
        it->mTurnaroundTime = 0;
        it->remainingTime = it->cpuBurstTime;
        
        intoReadyQ(*it);
        printf("time %dms: Process %c arrived %s\n", simu_time, it->proc_id, strReadyQ());
    }
}

// Then we check the ioVec to push processes into the readyQ if neccesary
void CPU::updateIOVec()
{
    list<Process> ioReadyProcess;
    list<Process>::iterator it = ioVec.begin();
    while ( it != ioVec.end())
    {
        if (it->arrivalTime <= simu_time)
        {
            ioReadyProcess.push_back(*it);
            ioVec.erase(it++);
        }
        else
            ++it;
    }
    ioReadyProcess.sort(compProcID);
    for (list<Process>::iterator it = ioReadyProcess.begin(); it != ioReadyProcess.end(); ++it)
    {
        it->remainingTime = it->cpuBurstTime;
        intoReadyQ(*it);
        printf("time %dms: Process %c completed I/O %s\n", simu_time, it->proc_id, strReadyQ());
    }
}

// At last, we update the mWaitTime and mTurnaroundTime of processes in the readyQ
void CPU::updateReadyQ()
{
    for (list<Process>::iterator it = readyQ.begin(); it != readyQ.end(); ++it)
    {
        it->mWaitTime++;
        it->mTurnaroundTime++;
    }
}

// The main simulation function
void CPU::simulate_RR(int t_cs, int t_slice, string method, const char* filename)
{
    // Check the method,
    if (method == "FCFS" || method == "RR")
        qMethod = "FCFS";
    else if (method =="SJF")
        qMethod = "SJF";
    
    // initialize the simulation variables
    simu_time = 0;
    cpuTimer = 0;
    numContextSwitch = 0;
    numPreemption = 0;
    state = "IDLE";
    unsigned numOfProcess = initialProcessesVec.size();
    
    printf("time %dms: Simulator started for %s %s\n", simu_time, method.c_str(), strReadyQ());
    // Start the iteration
    while (finishedVec.size() < numOfProcess || state != "IDLE")
    {
        updateInitialProcessVec();
        //updateIOVec();
        if (state == "IDLE")
        {
            updateIOVec();
            if (!readyQ.empty()) // Transition into SWITCH_IN
            {
                cpuProcess = readyQ.front();
                readyQ.pop_front();
                state = "SWITCH_IN";
                cpuTimer = t_cs/2-1; // In this time step, we are already in the SWITCH_IN state
                cpuProcess.mTurnaroundTime++;
                numContextSwitch++;
                updateReadyQ();
                simu_time++;
                continue;
            }
        }
        else if (state == "RUNNING")
        {
            if (cpuProcess.remainingTime>0 && cpuTimer>0) // Check if time slice expired or process finished
            {
                cpuProcess.remainingTime--;
                cpuTimer--;
                cpuProcess.mTurnaroundTime++;
                updateIOVec();
            }
            else
            {
                if (cpuProcess.remainingTime == 0) // kick the process out to I/O or finishedVec
                {
                    if (cpuProcess.numBurst > 1)
                        intoIOVec(cpuProcess);
                    else
                    {
                        finishedVec.push_back(cpuProcess);
                        printf("time %dms: Process %c terminated %s\n", simu_time, cpuProcess.proc_id, strReadyQ());
                    }
                    updateIOVec();
                }
                else if (cpuTimer == 0)  // Time slice expired but process not finished
                {                        // Kick it into readyQ
                    updateIOVec();
                    if (!readyQ.empty()) // If we have the next process in readyQ, go for the preemption!
                    {
                        intoReadyQ(cpuProcess);
                        printf("time %dms: Time slice expired; process %c preempted with %dms to go %s\n",
                               simu_time, cpuProcess.proc_id, cpuProcess.remainingTime, strReadyQ());
                        numPreemption++;
                    }
                    else                 // If readyQ is empty, then no preemption,
                    {                    // Pretend nothing happen in this step, restore the cpuTimer, run this step again.
                        printf("time %dms: Time slice expired; no preemption because ready queue is empty [Q empty]\n",
                               simu_time);
                        cpuTimer = t_slice;
                        continue;
                    }
                }
                // We are kicking the cpuProcess out either to the readyQ or to ioVec or to finishedVec
                // Then CPU is ready to accept waiting processes in the readyQ
                // check if anyone is waiting in the readyQ
                if (!readyQ.empty())
                {
                    state = "SWITCH_FULL";
                    cpuProcess = readyQ.front();
                    readyQ.pop_front();
                    cpuTimer = t_cs-1;
                    cpuProcess.mTurnaroundTime++;
                    numContextSwitch++;
                }
                else
                {
                    state = "SWITCH_OUT";
                    cpuTimer = t_cs/2-1;
                    //numContextSwitch++; // you don't need to count SWITCH_OUT into numContextSwitch
                }
            }
        }
        else if (state == "SWITCH_IN")
        {
            if (cpuTimer == 0)
            {
                state = "RUNNING";
                printf("time %dms: Process %c started using the CPU %s\n", simu_time, cpuProcess.proc_id, strReadyQ());
                cpuTimer = t_slice-1;
                cpuProcess.remainingTime--;
                cpuProcess.mTurnaroundTime++;
            }
            else
            {
                cpuTimer--;
                cpuProcess.mTurnaroundTime++;
            }
            updateIOVec();
        }
        else if (state == "SWITCH_OUT")
        {
            if (cpuTimer==0)
            {
                if (readyQ.empty())
                {
                    state = "IDLE";
                    cpuTimer = -1;
                }
                else // If some process is ready during the SWITCH_OUT, then we still do SWITCH_IN
                {
                    state = "SWITCH_IN";
                    cpuProcess = readyQ.front();
                    readyQ.pop_front();
                    cpuTimer = t_cs/2-1;
                    cpuProcess.mTurnaroundTime++;
                    numContextSwitch++;
                }
            }
            else
                cpuTimer--;
            updateIOVec();
        }
        else if (state == "SWITCH_FULL")
        {
            if(cpuTimer==0)
            {
                state = "RUNNING";
                printf("time %dms: Process %c started using the CPU %s\n", simu_time, cpuProcess.proc_id, strReadyQ());
                cpuTimer = t_slice-1;
                cpuProcess.remainingTime--;
                cpuProcess.mTurnaroundTime++;
            }
            else
            {
                cpuTimer--;
                cpuProcess.mTurnaroundTime++;
            }
            
            updateIOVec();
        }
        updateReadyQ();
        simu_time++;
    }
    printf("time %dms: Simulator ended for %s\n", simu_time-1, method.c_str());
    
    // Calculate the statistics
    double avg_waitTime=0;
    double avg_turnaroundTime=0;
    double avg_cpuBurstTime=0;
    int total_numBurst = 0;
    for (list<Process>::iterator it = finishedVec.begin(); it != finishedVec.end(); ++it)
    {
        avg_waitTime += it->mWaitTime;
        avg_turnaroundTime += it->mTurnaroundTime;
        avg_cpuBurstTime += it->cpuBurstTime * it->initialnumBurst;
        total_numBurst += it->initialnumBurst;
    }
    
    // Write statistics to file
    FILE *file = fopen(filename, "a");
    fprintf(file,
            "Algorithm %s\n-- average CPU burst time: %.2f ms\n-- average wait time: %.2f ms\n-- average turnaround time: %.2f ms\n-- total number of context switches: %d\n-- total number of preemptions: %d\n"
            ,method.c_str(),
            avg_cpuBurstTime/total_numBurst,
            avg_waitTime/total_numBurst,
            avg_turnaroundTime/total_numBurst,
            numContextSwitch,
            numPreemption);
    fclose(file);
}

int main(int argn, char** argv)
{
    if (argn != 3)
    {
        fprintf(stderr, "ERROR: Invalid arguments\nUSAGE: ./a.out <input-file> <stats-output-file>\n");
        return EXIT_FAILURE;
    }
    
    // initialize the output file
    const char* filename = argv[2];
    FILE* file = fopen(filename, "w");
    fclose(file);
    
    vector<Process> all_processes;
    ifstream in(argv[1]);
    
    string line;
    while(getline(in, line))
    {
        if (line.empty() || line[0] == '#' || line[0] == ' ')
            continue;
        else
        {
            Process proc;
            proc.readInput(line);
            all_processes.push_back(proc);
        }
    }
    
    CPU cpuFCFS(all_processes);
    cpuFCFS.simulate_RR(8, 1000000, "FCFS", filename);
    
    cout<<endl;
    CPU cpuSJF(all_processes);
    cpuSJF.simulate_RR(8, 10000000, "SJF", filename);
    
    cout<<endl;
    CPU cpuRR(all_processes);
    cpuRR.simulate_RR(8, 84, "RR", filename);
    
    return EXIT_SUCCESS;
}