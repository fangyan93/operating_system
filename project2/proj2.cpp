#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <utility>
#include <limits>  

#define NUM_MEM_FRAME 256
#define FRAME_PER_LINE 32

using namespace std;

typedef list < pair <int, int> > List_of_Jobs;

class Process
{
public:
    Process(string line);
    char procID;
    // pair <int, int> time; 
    int memorySize;
	int startIndex; // record the starting physical address for contiguous
    bool finished; // become true when all jobs are done
    /* For contiguous memory management */
    int memFrameStart; // The starting frame when it's in memory

    /* For non-contiguous memory management */
    vector<int> pageTable; // A page table that's mapping the logical address of the process
                           // onto the physical address in memory
    List_of_Jobs listOfJobs;
};

bool operator<(Process A, Process B)
{
	return A.procID < B.procID;
}


Process::Process(string line)
{
	finished = false;
    istringstream iss(line);
    string tmp;
    getline(iss, tmp, ' ');
    procID = tmp.c_str()[0];
    getline(iss, tmp, ' ');
    memorySize = atoi(tmp.c_str());
    cout << memorySize << endl;
    while (getline(iss,tmp, ' '))
    {
    	size_t pos = tmp.find_first_of('/');
    	string str1 = tmp.substr(0,pos);
    	string str2 = tmp.substr(pos+1, tmp.length() - pos - 1);
    	pair <int,int> job_spec;
    	job_spec.first = atoi(str1.c_str());
    	job_spec.second = atoi(str2.c_str());
    	listOfJobs.push_back(job_spec);
    }
}


// The class for the memory management system
class MMS
{
public:
	MMS(int num, list<Process> allProcesses);
	void printMMS();
	int findNextAvailable(int start); // Find the next available frame
	int findNextOccupied(int start); // find the next occupied frame
	int numAvailable; // count how many available slots left, use to trigger defrag
					  // +1 when remove 1 char, -1 when place 1 char, initially 256
	int numProcess; // number of process, specified in input file
	string method; //区分noncontiguous, next-fit, best-fit, worse-fit
	
	int defragmentation(); // 
	void contiguous(); // 
	void nonContiguous(); //
	bool nonConti_place(vector<int> & pageTable, char procID, int num);
	
	
	bool checkStart(int t); //check if the current process arrives
	bool checkEnd(int t); //check if a process is terminating
	//variable
	list<Process> restProcess; //store all processes waiting for next execution
	char mem[NUM_MEM_FRAME]; // characters for reperesenting all the memory frames
	list<Process> onRunning;
	list<Process> finishedProcess; // store all process finished
};

// Initialization of the MMS
MMS::MMS(int num, list<Process> allProcesses)
{
	numAvailable = NUM_MEM_FRAME;
	numProcess = num;
	restProcess = allProcesses;
	cout << "constructor " << restProcess.size() << endl;
	for (int i = 0; i < NUM_MEM_FRAME; i++)
		mem[i] = '.';
	
}

// Print the state of MMS
void MMS::printMMS()
{
	for (int i = 0; i< FRAME_PER_LINE; i++)
		cout << "=";
	cout << endl;

	int line_start = 0;
	for (int i = 0; i < NUM_MEM_FRAME/FRAME_PER_LINE; i++)
	{
		for (int j = 0; j < FRAME_PER_LINE; j++)
			cout << mem[line_start+j];
		line_start += FRAME_PER_LINE;
		cout << endl;
	}

	for (int i = 0; i< FRAME_PER_LINE; i++)
		cout << "=";
	cout << endl;
};
//return -1 when pos equals to NUM_MEM_FRAME not NUM_MEM_FRAME - 1 ?
int MMS::findNextAvailable(int start)
{
	int pos = start;
	while (1)
	{
		if (pos == NUM_MEM_FRAME-1)
			return -1;

		if (mem[pos] == '.')
			break;
		else
			pos++;
	}

	return pos;
}

int MMS::findNextOccupied(int start)
{
	int pos = start;
	while (1)
	{
		if (pos == NUM_MEM_FRAME-1)
			return -1;
		if (mem[pos] != '.')
			break;
		else
			pos++;
	}
	return pos;
}

//Note that, when defragmentation, all processes are suspended, so, the 
//running time of all processes should increase during defragmentation
//for example, if defragmentation takes 100ms, and A was running before with 50 
//running time, after defragmentation, its running time should become 150,
//otherwise checkEnd function does not work
//start time remains unchanged.

int MMS::defragmentation()
//void MMS::defragmentation()
{
	int availableStart = 0;
	//int availableEnd = 0;
	int occupiedStart = 0;
	int occupiedEnd = 0;
	int numMovedFrame = 0;
	while (1)
	{
		availableStart = findNextAvailable(availableStart);
		if (availableStart == -1) // End of frames
			break;
		occupiedStart = findNextOccupied(availableStart);
		if (occupiedStart == -1)
			break;
		occupiedEnd = findNextAvailable(occupiedStart)-1;
		
		if (occupiedEnd < 0)
			occupiedEnd = NUM_MEM_FRAME-1;
		
		// Time to move!
		int replaceEnd = availableStart + occupiedEnd - occupiedStart;
		char proc_id = mem[occupiedStart];
		for (int i = availableStart; i <= replaceEnd; i++)
			mem[i] = proc_id;
		for (int i = replaceEnd+1; i <= occupiedEnd; i++)
			mem[i] = '.';

		// Move the position back for next iteration
		availableStart = replaceEnd;
		numMovedFrame += occupiedEnd - occupiedStart + 1; 
	}
	cout << "defragmentation is done!\n";
	return numMovedFrame;
}

void MMS::nonContiguous(){
	//all files has been loaded.
	int t = 0;
	bool all_finished = false;
	list<Process>::iterator ita = finishedProcess.begin();
	cout << "noncontiguous " << restProcess.size() << endl; 
	method = "noncontiguous";
	while(t >= 0 && !all_finished ){//simulation starts
		// cout << "TIME !!!!!! " << t << endl;
		checkEnd(t); //move ending process to restRocesses / finished
			//remove places occupied
		//}	
		// cout << "ll" << endl;
		checkStart(t);
			//find places for # of frames
		if( finishedProcess.size() == numProcess ){
			all_finished = true;
		}
		t++;
			//if can not find, continue to next process, i.e. deny current request
	}
	for(; ita != finishedProcess.end(); ita++){
		cout << ita->procID << endl;
	}
	// for(; ita != restProcess.end(); ita++){
	// 	cout << ita->procID << endl;
	// }
	cout << onRunning.size() << " " << restProcess.size() << " " << finishedProcess.size() << endl;
}

bool MMS::checkStart(int t){
	//check all restProcesses, put any starting process into onRunning
	list<Process>::iterator it = restProcess.begin();
	bool found = false;
	list<Process> toBeErased;
	while(it != restProcess.end() ){
		found = false;
		// cout << "procID   " <<it->procID << endl;
		List_of_Jobs::iterator jobit = it->listOfJobs.begin();
		for(; jobit != it->listOfJobs.end(); jobit++){
			// cout << "dd333" << endl;
			if(jobit->first == t){
				cout << "time "<< t <<"ms: Process " 
						 << it->procID << " arrived (requires " << it->memorySize << " frames)" << endl;
				// if(it->memorySize <= this->numAvailable ){
				if(method == string("noncontiguous")){
					cout << it->memorySize << " " << it->procID << endl;
					it->pageTable.clear(); // refresh pageTable, maybe it has been changed by other method
					if(nonConti_place(it->pageTable, it->procID, it->memorySize)){							cout << it->pageTable.size() << endl;
						cout << "time "<< t <<"ms: Placed process " << it->procID << ":" << endl;
						onRunning.push_back(*it);
						restProcess.erase(it++);
						this->printMMS();
					}else{
						cout << "time "<< t <<"ms: Cannot place process " 
						 << it->procID << " -- skipped!" << endl;
						 if(it->listOfJobs.size() == 1){
						 	finishedProcess.push_back(*it);
						 	restProcess.erase(it++);
						 }
						break;
					}
						
				}
					
					// cout << "dd" << endl;
				// }else{
					
				// }
				
				found = true;
				break;
			}
		}
		if(found == false){
			it++;
		}
	}
	return found;
}

bool MMS::checkEnd(int t){
	//check all onRunning processes, put any ending process into rest if not finished,
	//into finishedProcess if it's finished
	// cout << "dd1" << endl;
	list<Process>::iterator it = onRunning.begin();
	bool found = false;
	while(it != onRunning.end()){
		bool Yes = false;
		List_of_Jobs::iterator jobit = it->listOfJobs.begin();
		// cout << "dd2" << endl;
		while(jobit != it->listOfJobs.end()){
			if(jobit->second + jobit->first == t){
				if(it->listOfJobs.size() > 1){
					jobit = it->listOfJobs.erase(jobit++);
					restProcess.push_back(*it);
					cout << "push_back one in rest" << it->procID << endl;
				}else if(it->listOfJobs.size() <= 1){
					jobit = it->listOfJobs.erase(jobit++);
					it->finished = true;
					finishedProcess.push_back(*it);
				}
				cout << "dd3 " << it->pageTable.size() << endl;

				for(int i = 0; i < it->pageTable.size(); i++){
					mem[it->pageTable[i]] = '.';
					numAvailable += 1;
				}
				cout << "time " << t << "ms: Process " << it->procID << " removed" << endl;
				this->printMMS();
				onRunning.erase(it++);
				found = true;
				Yes = true;
				break;
			}else{
				jobit++;
			}
		}
		if(Yes == false){
			it++;
		}
		
	}

	return found;
}

bool MMS::nonConti_place(vector<int> & pageTable, char procID, int num){
	int availableStart = 0;
	//int availableEnd = 0;
	int occupiedStart = 0;
	int occupiedEnd = 0;
	int count = 0;
	if(numAvailable < num){
		return false;
	}
	cout <<  "dddidi  "<< count << "  " << num << " " << procID << endl;
	while(count < num){
		availableStart = findNextAvailable(availableStart);
		
		// if (availableStart == -1) // End of frames
		// 	break;
		occupiedStart = findNextOccupied(availableStart);
		if (occupiedStart < 0)
			occupiedStart = NUM_MEM_FRAME-1;
		if (occupiedStart == -1)
			break;
		
		
		cout << availableStart << endl;
		cout << occupiedStart << endl;
		for(int i = availableStart; i < occupiedStart && count < num; i++){
			mem[i] = procID;
			pageTable.push_back(i);
			count++;
			numAvailable -= 1;
		}

		
	}
	return true;
}
//定义一个method，区分noncontiguous，next-fit，best-fit，worst-fit
//这样checkStart和checkEnd就可以共用了
void simulation(const vector<int> & pages, const string & method); 
// void LFU(const vector<int> & pages);
// void LRU(const vector<int> & pages);
int futureForward(const vector<int> & pages, int current, vector<int>& mem);
int RecentBackward(const vector<int> & pages, int current, vector<int>& mem);
int FrequentBackward(map<int, int> & forLFU, const vector<int>& pages, vector<int>& mem);

void virtual_memory(const vector<int> & pages){
	simulation(pages, "OPT");
	cout << endl << endl;
	simulation(pages, "LRU");
	cout << endl << endl;
	simulation(pages, "LFU");
	cout << endl;
}

void simulation(const vector<int> & pages, const string& method){
	int numOfVictim = 0;
	vector<int> mem(3,0);
	cout << "Simulating " << method << " with fixed frame size of 3" << endl;
	map<int, int> forLFU;
	for(int i = 0; i < pages.size(); i++){
		bool hasvictim = true;//if there is victim page
		bool newitem = false;//if unused memory is being used
		for(int j = 0; j < 3; j++){
			if(mem[j] == pages[i]){
				hasvictim = false;
				break;
			}
			if(mem[j] == 0){
				mem[j] = pages[i];
				newitem = true;
				break;
			}
		}
		if(hasvictim){
			numOfVictim++;
			int victimpage = 0;
			if(!newitem && method == "OPT"){
				int index = futureForward(pages, i, mem);
				victimpage = mem[index];
				mem[index] = pages[i];
			}
			if(!newitem && method == "LRU"){
				int index = RecentBackward(pages, i, mem);
				if(index != -1){
					victimpage = mem[index];
					mem[index] = pages[i];
				}
			}
			if(!newitem && method == "LFU"){
				int index = FrequentBackward(forLFU, pages, mem);
				victimpage = mem[index];
				mem[index] = pages[i];
				forLFU[victimpage] = 0;
			}
			cout << "referencing page " << pages[i] << " [mem:" ;
			for(int j = 0; j < 3; j++){
				if(mem[j] == 0 ){
					cout << " .";
				}
				else{
					cout << " " << mem[j];
				}		
			}
			cout << "] PAGE FAULT (";
			if(!newitem ){
				cout << "victim page " << victimpage;
			}else{
				cout << "no victim page";
			}
			cout << ")" << endl;
		}
		forLFU[pages[i]]++;
	}

	cout << "End of " << method <<  " simulation ("  << numOfVictim << " page faults)";
}
int futureForward(const vector<int> & pages, int current, vector<int>& mem){
	vector<pair<bool, int> > index(3, make_pair(false, 0));
	int count = 0;
	int victimIndex = 0;
	for(int i = current + 1; i < pages.size();i++){
		for(int j = 0; j < 3; j++){
			if(mem[j] == pages[i] && index[j].first == false){
				index[j].first = true;
				index[j].second = i;
				count++;
			}
		}
		if(count == 3){
			break;
		}
		
	}
	int max = 0;
	map<int , int> noInFuture;
	for(int i = 0; i < 3; i++){
		if(index[i].second > max){
			max = index[i].second;
			victimIndex = i;
		}
		if(index[i].first == false){
			noInFuture[mem[i]] = i; 
		}
	}
	if(noInFuture.size() > 0){
		return noInFuture.begin()->second;
	}
	return victimIndex;
}

int RecentBackward(const vector<int> & pages, int current, vector<int>& mem){
	vector<pair<int, bool> > index(3, make_pair(0, false));
	int count = 0;
	int victimIndex = 0;
	bool found = false;
	int min = pages.size();
	//here current >= 3, because otherwise this function will not be called, !newitem holds true
	for(int i = current - 1 ; i >= 0;i--){
		for(int j = 0; j < 3; j++){
			if(mem[j] == pages[i] && index[j].second == false){
				index[j].second = true;
				index[j].first = i;
				if(min > i){
					min = i;
					victimIndex = j;
				}
				count++;
				found = true;
			}
		}
		if(count == 3){
			break;
		}
		
	}
	if(found){
		return victimIndex;
	}
	return -1;
	
}

int FrequentBackward(map<int,int> & forLFU, const vector<int>& pages, vector<int>& mem){
	int min = numeric_limits<int>::max();
	int index = 0;
	for(int i = 0; i < 3; i++){
		if(min >= forLFU[mem[i]]){
			if(min == forLFU[mem[i]]){
				index = mem[i] > mem[index] ? index:i;
			}else{
				min = forLFU[mem[i]];
				index = i;
			}
		}
	}
	return index;
	return 0;
}



int main(int argc, char* argv[])
{
	//argv[1] contiguous/noncontiguous input file
	//argv[2] virtual memeory input file
	list<Process> allProcesses;
	ifstream in(argv[1]);
	string line;
	string virtual_line;
	getline(in, line);
	int numOfProcess = atoi(line.c_str());

	while (getline(in, line))
	{
		Process proc = Process(line);
		allProcesses.push_back(proc);
	}

	MMS mms(numOfProcess, allProcesses);
	// // for (i  
	mms.nonContiguous();
	// mms.printMMS();
	// // mms.defragmentation();




	// ifstream vm(argv[2]);
	// string line;
	// int page_num;
	// string page;
	// // map<int, int> after;
	// // map<int, int> before;
	// vector<int> pages;
	// while(getline(vm, line)){
	// 	istringstream ss(line);
	// 	while(getline(ss, page, ' ')){
	// 		page_num = atoi(page.c_str());
	// 		pages.push_back(page_num);
	// 	}
		
	// }
	// virtual_memory(pages);
	// void virtual_memory(ifstream& instr);
	// // mms.printMMS();
	return 0;
	
	
	
	//大致框架
	//list<Process> tmp = mms.restProcess;//因为process在end的时候会erase element in listofjobs
	
	//mms.method = "next-fit";
	//mms.contiguous() 或mms.next_fit() 建议把contiguous三个方法分开写
	//mms.restProcess = tmp;
	//mms.onRunning.clear();
	//mms.finishedProcess.clear();
	
	//mms.method = "noncontiguous";
	//mms.nonContiguous();
	//mms.restProcess = tmp;
	//mms.onRunning.clear();
	//mms.finishedProcess.clear();
}













