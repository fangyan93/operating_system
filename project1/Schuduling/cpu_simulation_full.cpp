//
//  cpu_simulation.cpp
//
//
//  Created by fangyan xu on 10/9/16.
//
//

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
    /* The state is either READY, or RUNNING, or BLOCKED, or NOT_ARRIVED */
    string state;
    int mcpuBurstTime, mTurnaroundTime, mWaitTime; // Time that we need to measure
    int remainingTime; // The count down (remaining time) for running or blocking
    // We need to intialize this upon every start of a running
    // or blocking process
    string proc_id;
    int num_in_queue; // deal with processes with the same arrival time
    int arrivalTime, cpuBurstTime, numBursts, ioTime;
    Process();
    bool finish;
};

// Constructor of processes
Process::Process()
{
    // initialize the state of all processe
    state = "NOT_ARRIVED";
    // Set measures to 0
    mcpuBurstTime = 0;
    mTurnaroundTime = 0;
    mWaitTime = 0;
    remainingTime = 0;
    finish = false;
}

bool operator<(const Process& p1,const Process& p2){
    
    if (p1.arrivalTime < p2.arrivalTime) {
        return true;
    }
    else if(p1.arrivalTime == p2.arrivalTime){
        if (p1.arrivalTime == p2.arrivalTime && p1.num_in_queue < p2.num_in_queue) {
            return true;
        }
    }
    return false;
}


bool operator==(const Process& p1,const Process& p2){
    return (p1.arrivalTime == p2.arrivalTime && p1.num_in_queue == p2.num_in_queue);
}

class CPU
{
public:
    CPU(vector<Process> all_input_processes);
    void simulate_FCFS(int t_cs, string& output);
    void simulate_SJF(int t_cs, string& output);
    void simulate_RR(int t_cs, int t_slice, string& output);
    vector<Process> initialProcessesVec; /* all_processes */
    list<Process> readyQ;
    list<Process> ioVec;
    list<Process> finishedVec;
    Process* process_ptr;
    string state; /* can be RUNNING, CONTEXT_SWITCH or IDLE */
    int timer;
    int numContSwitch;
};

CPU::CPU(vector<Process> all_input_processes) // the constructor of CPU
{
    initialProcessesVec = all_input_processes;
    process_ptr = NULL;
    state = "IDLE";
    timer = 0;
    numContSwitch = 0;
}

void CPU::simulate_FCFS(int t_cs, string& output){
    int time = 0;
    int switchtime = t_cs/2;
    bool FINISH = false;
    while(time >= 0 && !(FINISH)){
        if (time == 0) {
            cout << "time " << time << "ms: Simulator started for FCFS" << endl;
        }
        if (this->state == "CONTEXT_SWITCH") {
            for(list<Process>::iterator it = ioVec.begin(); it != ioVec.end(); it++){
                (*it).remainingTime--;
                if((*it).remainingTime == 0){
                    (*it).state = "READY";
                    readyQ.push_back(*it);
                    it = --ioVec.erase(it);
                }
            }
            time++;
            switchtime--;
            if (switchtime == 1) {
                this->state = "RUNNING";
                switchtime = t_cs/2;
            }
            continue;
        }
        //in each iteration, update readyQ if there is new arrival
        for(int i = 0; i < initialProcessesVec.size(); i++){
            if(initialProcessesVec[i].arrivalTime == time &&
               initialProcessesVec[i].state != "RUNNING"){
                readyQ.push_back(initialProcessesVec[i]);
                this->state = "RUNNING" ;
                cout << "time " << time << "ms: Process " <<initialProcessesVec[i].proc_id << " arrived [Q";
                if (readyQ.size() > 0) {
                    for(list<Process>::iterator it = readyQ.begin(); it != readyQ.end(); it ++){
                        cout << " " << (*it).proc_id ;
                    }
                }
                else{
                    cout << " empty";
                }
                cout << "]"<< endl;
                
                
            }
        }
        //for a blocking process，remainingTime-1
        if (this->state == "RUNNING") {
            for(list<Process>::iterator it = ioVec.begin(); it != ioVec.end(); it++){
                if (it->numBursts != 0) {
                    //                cout << "ioVec " << it->proc_id << " " <<  (*it).remainingTime<< endl;
                    (*it).remainingTime--;
                    
                    
                }
                else{
                    continue;
                }
                
            }
        }
        
        
        //if process complete blocking, back to readyQ
        if (this->state == "RUNNING") {
            for(list<Process>::iterator it = ioVec.begin(); it != ioVec.end(); it++){
                if (it->numBursts != 0) {
                    if((*it).remainingTime == 0){
                        (*it).state = "READY";
                        (*it).arrivalTime = time;
                        readyQ.push_back(*it);
                        cout << "time " << time << "ms: Process " << (*it).proc_id << " completed I/O [Q";
                        if (readyQ.size() > 0) {
                            for(list<Process>::iterator itr = readyQ.begin(); itr != readyQ.end(); itr ++){
                                cout << " " << (*itr).proc_id ;
                            }
                        }
                        else{
                            cout << " empty";
                        }
                        cout << "]"<< endl;
                        it = ioVec.erase(it);
                        
                    }
                }
                else{
                    continue;
                }
            }
        }
        //if one process is running, let process_ptr points to
        //the process be executed next
        if(process_ptr == NULL && readyQ.size() != 0 && (this->state == "RUNNING" ||this->state == "IDLE")) {
            list<Process>::iterator it = readyQ.begin();
            process_ptr = new Process;
            *process_ptr = *it;
            
            for(it = readyQ.begin(); it != readyQ.end(); it++){
                if((*it) < (*process_ptr)){
                    *process_ptr = *it;
                }
            }
            for(it = readyQ.begin(); it != readyQ.end(); it++){
                if (*process_ptr == *it) {
                    readyQ.erase(it);
                    break;
                }
            }
            process_ptr->remainingTime = process_ptr->cpuBurstTime + 1;
            process_ptr->state = "RUNNING";
            
            process_ptr->mWaitTime += time - process_ptr->arrivalTime  ;
            this->state = "CONTEXT_SWITCH";
            cout << "time " << time + switchtime << "ms: Process " << process_ptr->proc_id  << " started using the CPU [Q";
            if (readyQ.size() > 0) {
                for(list<Process>::iterator it = readyQ.begin(); it != readyQ.end(); it ++){
                    cout << " "<<(*it).proc_id;
                }
            }
            else{
                cout << " empty";
            }
            cout << "]" <<endl;
        }
        
        //run the process for 1 millisecond, mcpuBurstTime +1
        //remainingTime - 1
        if (process_ptr != NULL && this->state == "RUNNING") {
            process_ptr->mcpuBurstTime++;
            process_ptr->remainingTime--;
            //for a process completed, put it in the ioVec，let remainingTime to its ioTime
            //then process_ptr = NULL
            if(process_ptr->remainingTime == 0){
                process_ptr->remainingTime = process_ptr->ioTime;
                process_ptr->state = "BLOCKED";
                process_ptr->numBursts--;
                if (process_ptr->numBursts == 0) {
                    process_ptr->finish = true;
                    cout << "time " << time << "ms: Process " << process_ptr->proc_id  << " terminated [Q";
                    if (readyQ.size() > 0) {
                        for(list<Process>::iterator it = readyQ.begin(); it != readyQ.end(); it ++){
                            cout  << " " << (*it).proc_id;
                        }
                    }
                    else{
                        cout << " empty";
                    }
                    cout <<"]" <<  endl;
                }
                else{
                    cout << "time " << time << "ms: Process " << process_ptr->proc_id  << " completed a CPU burst; "
                    << process_ptr->numBursts  ;
                    cout << " to go [Q";
                    if (readyQ.size() > 0) {
                        for(list<Process>::iterator it = readyQ.begin(); it != readyQ.end(); it ++){
                            cout  << " " << (*it).proc_id;
                        }
                    }
                    else{
                        cout << " empty";
                    }
                    cout <<"]" <<  endl;
                    cout << "time " << time << "ms: Process " << process_ptr->proc_id  << " blocked on I/O until time " << time + process_ptr->ioTime << "ms [Q";
                    if (readyQ.size() > 0) {
                        for(list<Process>::iterator it = readyQ.begin(); it != readyQ.end(); it ++){
                            cout  << " " << (*it).proc_id;
                        }
                    }
                    else{
                        cout << " empty";
                    }
                    cout <<"]" <<  endl;
                }
                
                process_ptr->mTurnaroundTime += time - process_ptr->arrivalTime;
                process_ptr->mcpuBurstTime -= 1;
                ioVec.push_back(*process_ptr);
                delete process_ptr;
                process_ptr = NULL;
                this->state = "CONTEXT_SWITCH" ;
                numContSwitch++;
                if(process_ptr == NULL && readyQ.size() != 0 ) {
                    list<Process>::iterator it = readyQ.begin();
                    Process* tmp = new Process;
                    *tmp = *it;
                    for(it = readyQ.begin(); it != readyQ.end(); it++){
                        if((*it) < (*tmp)){
                            *tmp = *it;
                        }
                    }
                    for(it = readyQ.begin(); it != readyQ.end(); it++){
                        if (*tmp == *it) {
                            (*it).mWaitTime -= switchtime;
                            break;
                        }
                    }
                }
            }
        }
        
        if (this->state != "CONTEXT_SWITCH") {
            int count = 0;
            for (list<Process>::iterator it = ioVec.begin(); it != ioVec.end(); it ++) {
                if (it->finish) {
                    count++;
                }
                
            }
            if (count == initialProcessesVec.size()) {
                FINISH = true;
                cout << "time " << time << "ms: Simulator ended for FCFS" << endl;
                this->state = "IDLE";
            }
        }
        
        time++;
    }
    
    string str = output;
    ofstream out(str);
    float turnarounds = 0;
    for (int i = 0 ; i < initialProcessesVec.size(); i++) {
        turnarounds += initialProcessesVec[i].numBursts ;
    }
    
    float totalburst = 0;
    for (list<Process>::iterator it = ioVec.begin(); it != ioVec.end(); it ++) {
        totalburst += it->mcpuBurstTime;
    }
    float average_cpuburst = totalburst/turnarounds;
    float totalwait = 0;
    for (list<Process>::iterator it = ioVec.begin(); it != ioVec.end(); it ++) {
        totalwait += it-> mWaitTime;
    }
    
    float average_wait = totalwait /turnarounds;
    
    
    float totalturntime = 0;
    for (list<Process>::iterator it = ioVec.begin(); it != ioVec.end(); it ++) {
        totalturntime += it->mTurnaroundTime;
    }
    float average_turnaround = totalturntime/turnarounds;
    
    out << "Algorithm FCFS\n" ;
    out << "-- average CPU burst time: "  << fixed << setprecision(2)<< average_cpuburst << endl;
    out << "-- average wait time: " << setprecision(2)<< average_wait << endl;
    out << "-- average turnaround time: "<< setprecision(2) << average_turnaround << endl;
    out << "-- total number of context switches: " << numContSwitch << endl;
    out << "-- total number of preemmption: " << 0 << endl;
}









void CPU::simulate_SJF(int t_cs, string& output){
    int time = 0;
    int switchtime = t_cs/2;
    bool FINISH = false;
    while(time >= 0 && !(FINISH)){
        if (time == 0) {
            cout << "time " << time << "ms: Simulator started for SJF" << endl;
        }
        if (this->state == "CONTEXT_SWITCH") {
            for(list<Process>::iterator it = ioVec.begin(); it != ioVec.end(); it++){
                (*it).remainingTime--;
                if((*it).remainingTime == 0){
                    (*it).state = "READY";
                    readyQ.push_back(*it);
                    it = --ioVec.erase(it);
                }
            }
            time++;
            switchtime--;
            if (switchtime == 1) {
                this->state = "RUNNING";
                switchtime = t_cs/2;
            }
            continue;
        }
        //in each iteration, update readyQ if there is new arrival
        for(int i = 0; i < initialProcessesVec.size(); i++){
            if(initialProcessesVec[i].arrivalTime == time &&
               initialProcessesVec[i].state != "RUNNING"){
                readyQ.push_back(initialProcessesVec[i]);
                this->state = "RUNNING" ;
                cout << "time " << time << "ms: Process " <<initialProcessesVec[i].proc_id << " arrived [Q";
                if (readyQ.size() > 0) {
                    for(list<Process>::iterator it = readyQ.begin(); it != readyQ.end(); it ++){
                        cout << " " << (*it).proc_id ;
                    }
                }
                else{
                    cout << " empty";
                }
                cout << "]"<< endl;
                
                
            }
        }
        //for blocking processes，remainingTime-1
        if (this->state == "RUNNING") {
            for(list<Process>::iterator it = ioVec.begin(); it != ioVec.end(); it++){
                if (it->numBursts != 0) {
                    //                cout << "ioVec " << it->proc_id << " " <<  (*it).remainingTime<< endl;
                    (*it).remainingTime--;
                    
                    
                }
                else{
                    continue;
                }
                
            }
        }
        
        
        //if process complete block, back to reayQ
        if (this->state == "RUNNING") {
            for(list<Process>::iterator it = ioVec.begin(); it != ioVec.end(); it++){
                if (it->numBursts != 0) {
                    if((*it).remainingTime == 0){
                        (*it).state = "READY";
                        (*it).arrivalTime = time;
                        readyQ.push_back(*it);
                        cout << "time " << time << "ms: Process " << (*it).proc_id << " completed I/O [Q";
                        if (readyQ.size() > 0) {
                            for(list<Process>::iterator itr = readyQ.begin(); itr != readyQ.end(); itr ++){
                                cout << " " << (*itr).proc_id ;
                            }
                        }
                        else{
                            cout << " empty";
                        }
                        cout << "]"<< endl;
                        it = ioVec.erase(it);
                        
                    }
                }
                else{
                    continue;
                }
            }
        }
        //if one process is running, let process_ptr points to
        //the process be executed next
        if(process_ptr == NULL && readyQ.size() != 0 && (this->state == "RUNNING" ||this->state == "IDLE")) {
            list<Process>::iterator it = readyQ.begin();
            process_ptr = new Process;
            *process_ptr = *it;
            
            for(it = readyQ.begin(); it != readyQ.end(); it++){
                if((*it).cpuBurstTime < (*process_ptr).cpuBurstTime){
                    *process_ptr = *it;
                }
            }
            for(it = readyQ.begin(); it != readyQ.end(); it++){
                if (*process_ptr == *it) {
                    readyQ.erase(it);
                    break;
                }
            }
            process_ptr->remainingTime = process_ptr->cpuBurstTime + 1;
            process_ptr->state = "RUNNING";
            
            process_ptr->mWaitTime += time - process_ptr->arrivalTime  ;
            this->state = "CONTEXT_SWITCH";
            cout << "time " << time + switchtime << "ms: Process " << process_ptr->proc_id  << " started using the CPU [Q";
            if (readyQ.size() > 0) {
                for(list<Process>::iterator it = readyQ.begin(); it != readyQ.end(); it ++){
                    cout << " "<<(*it).proc_id;
                }
            }
            else{
                cout << " empty";
            }
            cout << "]" <<endl;
        }
        
        //run the process for 1 millisecond, mcpuBurstTime +1
        //remainingTime - 1
        if (process_ptr != NULL && this->state == "RUNNING") {
            process_ptr->mcpuBurstTime++;
            process_ptr->remainingTime--;
            //for a process completed, put it in the ioVec，let remainingTime to its ioTime
            //then process_ptr = NULL
            if(process_ptr->remainingTime == 0){
                process_ptr->remainingTime = process_ptr->ioTime;
                process_ptr->state = "BLOCKED";
                process_ptr->numBursts--;
                if (process_ptr->numBursts == 0) {
                    process_ptr->finish = true;
                    cout << "time " << time << "ms: Process " << process_ptr->proc_id  << " terminated [Q";
                    if (readyQ.size() > 0) {
                        for(list<Process>::iterator it = readyQ.begin(); it != readyQ.end(); it ++){
                            cout  << " " << (*it).proc_id;
                        }
                    }
                    else{
                        cout << " empty";
                    }
                    cout <<"]" <<  endl;
                    process_ptr->mTurnaroundTime += time - process_ptr->arrivalTime;
                    process_ptr->mcpuBurstTime -= 1;
                    finishedVec.push_back(*process_ptr);
                }
                else{
                    cout << "time " << time << "ms: Process " << process_ptr->proc_id  << " completed a CPU burst; "
                    << process_ptr->numBursts  ;
                    cout << " to go [Q";
                    if (readyQ.size() > 0) {
                        for(list<Process>::iterator it = readyQ.begin(); it != readyQ.end(); it ++){
                            cout  << " " << (*it).proc_id;
                        }
                    }
                    else{
                        cout << " empty";
                    }
                    cout <<"]" <<  endl;
                    cout << "time " << time << "ms: Process " << process_ptr->proc_id  << " blocked on I/O until time " << time + process_ptr->ioTime << "ms [Q";
                    if (readyQ.size() > 0) {
                        for(list<Process>::iterator it = readyQ.begin(); it != readyQ.end(); it ++){
                            cout  << " " << (*it).proc_id;
                        }
                    }
                    else{
                        cout << " empty";
                    }
                    cout <<"]" <<  endl;
                    process_ptr->mTurnaroundTime += time - process_ptr->arrivalTime;
                    process_ptr->mcpuBurstTime -= 1;
                    ioVec.push_back(*process_ptr);
                }
                
                
                delete process_ptr;
                process_ptr = NULL;
                this->state = "CONTEXT_SWITCH" ;
                numContSwitch++;
                if(process_ptr == NULL && readyQ.size() != 0 ) {
                    list<Process>::iterator it = readyQ.begin();
                    Process* tmp = new Process;
                    *tmp = *it;
                    for(it = readyQ.begin(); it != readyQ.end(); it++){
                        if((*it).cpuBurstTime < (*tmp).cpuBurstTime){
                            *tmp = *it;
                        }
                    }
                    for(it = readyQ.begin(); it != readyQ.end(); it++){
                        if (*tmp == *it) {
                            (*it).mWaitTime -= switchtime;
                            break;
                        }
                    }
                }
            }
        }
        
        if (this->state != "CONTEXT_SWITCH") {
            int count = 0;
            for (list<Process>::iterator it = finishedVec.begin(); it != finishedVec.end(); it ++) {
                if (it->finish) {
                    count++;
                }
                
            }
            if (count == initialProcessesVec.size()) {
                FINISH = true;
                cout << "time " << time << "ms: Simulator ended for SJF" << endl;
                this->state = "IDLE";
            }
        }
        
        time++;
    }
    
    string str = output;
    ofstream out(str);
    float turnarounds = 0;
    for (int i = 0 ; i < initialProcessesVec.size(); i++) {
        turnarounds += initialProcessesVec[i].numBursts ;
    }
    
    float totalburst = 0;
    for (list<Process>::iterator it = ioVec.begin(); it != ioVec.end(); it ++) {
        totalburst += it->mcpuBurstTime;
    }
    float average_cpuburst = totalburst/turnarounds;
    float totalwait = 0;
    for (list<Process>::iterator it = ioVec.begin(); it != ioVec.end(); it ++) {
        totalwait += it-> mWaitTime;
    }
    
    float average_wait = totalwait /turnarounds;
    
    
    float totalturntime = 0;
    for (list<Process>::iterator it = ioVec.begin(); it != ioVec.end(); it ++) {
        totalturntime += it->mTurnaroundTime;
    }
    float average_turnaround = totalturntime/turnarounds;
    
    out << "Algorithm SJF\n" ;
    out << "-- average CPU burst time: "  << fixed << setprecision(2)<< average_cpuburst << endl;
    out << "-- average wait time: " << setprecision(2)<< average_wait << endl;
    out << "-- average turnaround time: "<< setprecision(2) << average_turnaround << endl;
    out << "-- total number of context switches: " << numContSwitch << endl;
    out << "-- total number of preemmption: " << 0 << endl;
}










int main(int argc, char* argv[]){
    if(argc != 3){
        cerr << "ERROR: Invalid arguments\n" <<
        "USage: ./a.out <input-file> <stats-output-file>" << endl;
    }
    
    ifstream in(argv[1]);
    string line;
    std::vector<Process> all_process ;
    Process p;
    int num_in_queue = 0;
    while (getline(in, line))
    {
        if (line.empty()) {
            continue;
        }
        if(line[0] != '#' && line[0] != ' '){
            istringstream iss(line);
            string tmp;
            vector<string> Tmp;
            while(getline(iss, tmp, '|')){
                Tmp.push_back(tmp);
            }
            p.proc_id = Tmp[0];
            p.arrivalTime = atoi(Tmp[1].c_str());
            p.cpuBurstTime = atoi(Tmp[2].c_str());
            p.numBursts = atoi(Tmp[3].c_str());
            p.ioTime = atoi(Tmp[4].c_str());
            p.num_in_queue = num_in_queue;
            Tmp.clear();
            all_process.push_back(p);
            num_in_queue++;
        }
    }
    CPU cpu(all_process);
    
    string outfile = argv[2] ;
    //    cpu.simulate_FCFS(8, outfile);
    cpu.simulate_SJF(8, outfile);
    //    cpu.simulate_RR(8, 84, outfile);
    
    
}






