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
#include <set>
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
	int findNextAvailable(int start, int); // Find the next available frame
	int findNextOccupied(int start, int); // find the next occupied frame
	int numAvailable; // count how many available slots left, use to trigger defrag
					  // +1 when remove 1 char, -1 when place 1 char, initially 256
	int numProcess; // number of process, specified in input file
	string method; //区分noncontiguous, next-fit, best-fit, worse-fit
	int simuTimer;
	int cursor;
	int defragmentationTime;

	int defragmentation(); // 
	void contiguous(); // 
	void nonContiguous(); //
	int nextFit_place(char procID, int num);
	bool nonConti_place(vector<int> & pageTable, char procID, int num);
	bool worstFit_place(char procID, int num); //ID and required frame size
	bool bestFit_place(char procID, int num); //ID and required frame size
	
	//check if the current process arrives, move it onRunning
	bool checkStart(int t); 
	//check if a process is terminating, move it to rest or finished
	bool checkEnd(int t); 
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

int MMS::findNextAvailable(int start, int cursor = NUM_MEM_FRAME+1)
{
	int pos = start;
	while (1)
	{
		if (pos >= cursor)
			return -2;
		if (pos >= NUM_MEM_FRAME-1)
			return -1;
		if (mem[pos] == '.')
			break;
		else
			pos++;
	}

	return pos;
}

int MMS::findNextOccupied(int start, int cursor = NUM_MEM_FRAME+1)
{
	int pos = start;
	while (1)
	{
		if (pos >= NUM_MEM_FRAME-1)
			return -1;
		if (mem[pos] != '.')
			break;
		else
			pos++;
	}
	return pos;
}

int MMS::defragmentation()
{
	int availableStart = 0;
	int occupiedStart = 0;
	int occupiedEnd = 0;
	int numMovedFrame = 0;
	set<char> movedProcess;
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
		{
			if (mem[NUM_MEM_FRAME-1] == '.')
				occupiedEnd = NUM_MEM_FRAME-2;
			else
				occupiedEnd = NUM_MEM_FRAME-1;
		}
		
		if (occupiedEnd >= occupiedStart)
		{
		// Time to move!
		int replaceEnd = availableStart + occupiedEnd - occupiedStart;
		for (int i = availableStart; i <= replaceEnd; i++)
		{
			mem[i] = mem[ i - availableStart + occupiedStart ];

			//if (mem[i] != '.')
				movedProcess.insert(mem[i]);
		}
		for (int i = replaceEnd+1; i <= occupiedEnd; i++)
			mem[i] = '.';
		// Move the position back for next iteration
		availableStart = replaceEnd;
		numMovedFrame += occupiedEnd - occupiedStart+1;
		}
	}
	simuTimer += numMovedFrame;
	cout << "time " << simuTimer << "ms: Defragmentation complete (moved " << numMovedFrame << " frames: ";
	int i = 0;
	for (set<char>::iterator it = movedProcess.begin();
		it !=  movedProcess.end(); ++it)
	{
		cout << *it;
		if (i < (int)movedProcess.size()-1)
			cout << ", ";
		else
			cout << ")" << endl;
		i++;
	}
	printMMS();
	defragmentationTime += numMovedFrame;
	return numMovedFrame;
}

void MMS::nonContiguous()
{
	//all files has been loaded.
	defragmentationTime = 0;
	int t = 0;
	bool all_finished = false;
	list<Process>::iterator ita = finishedProcess.begin();
	method = "noncontiguous";
	cout << "time " << t << "ms: Simulator started (Non-contiguous)" << endl;
	while(t >= 0 && !all_finished ){//simulation starts
		checkEnd(t); 
		checkStart(t);
		if( (int) (finishedProcess.size()) == numProcess ){
			all_finished = true;
		}
		t++;
	}
	cout << "time " << t - 1 << "ms: Simulator ended (Non-contiguous)" << endl;
}

void MMS::contiguous(){
	//all files has been loaded.
	defragmentationTime = 0;
	simuTimer = 0;
	this->cursor = 0;
	bool all_finished = false;
	list<Process>::iterator ita = finishedProcess.begin();
	cout << "time " << simuTimer << "ms: Simulator started (Contiguous -- "<< method <<  ")" << endl;
	while(simuTimer >= 0 && !all_finished ){//simulation starts
		checkEnd(simuTimer); 
		checkStart(simuTimer);
		if( (int)(finishedProcess.size()) == numProcess ){
			all_finished = true;
		}
		simuTimer++;
	}
	cout << "time " << simuTimer - 1 << "ms: Simulator ended (Contiguous -- "<< method << ")" << endl;
}



bool compare_process(const Process &p1, const Process &p2){
	return p1.procID < p2.procID;
}
bool MMS::checkStart(int t){
	//check all restProcesses, put any starting process into onRunning
	restProcess.sort(compare_process);
	list<Process>::iterator it = restProcess.begin();
	bool found = false;
	list<Process> toBeErased;
	while(it != restProcess.end() ){//check all rest process
		found = false;
		List_of_Jobs::iterator jobit = it->listOfJobs.begin();
		while(jobit != it->listOfJobs.end()){
			if(jobit->first + defragmentationTime <= t){
				cout << "time "<< t <<"ms: Process " 
						 << it->procID << " arrived (requires " << it->memorySize << " frames)" << endl;
				//deal with different algorithm
				if(method == string("noncontiguous")){
					it->pageTable.clear(); // refresh pageTable, maybe it has been changed by other method
					if(nonConti_place(it->pageTable, it->procID, it->memorySize)){						
						cout << "time "<< t <<"ms: Placed process " << it->procID << ":" << endl;
						onRunning.push_back(*it);
						restProcess.erase(it++);
						this->printMMS();
					}else{
						cout << "time "<< t <<"ms: Cannot place process " 
						 << it->procID << " -- skipped!" << endl;
						 if(it->listOfJobs.size() > 1){
						 	it->listOfJobs.erase(jobit);
						 }else{
						 	it->listOfJobs.erase(jobit);
						 	finishedProcess.push_back(*it);
						 	restProcess.erase(it++);
						 }
						 this->printMMS();
					}
				}
				else if (method == string("Next-Fit"))
				{
					if (numAvailable >= it->memorySize)
					{
						it->startIndex = nextFit_place(it->procID, it->memorySize);	
						cout << "time "<< simuTimer <<"ms: Placed process " << it->procID << ":" << endl;
						onRunning.push_back(*it);
						restProcess.erase(it++);
						this->printMMS();
					}					
					else
					{
						cout << "time "<< simuTimer <<"ms: Cannot place process " 
						 << it->procID << " -- skipped!" << endl;
						 if(it->listOfJobs.size() > 1){
						 	it->listOfJobs.erase(jobit);
						 }else{
						 	it->listOfJobs.erase(jobit);
						 	finishedProcess.push_back(*it);
						 	restProcess.erase(it++);
						 }
						 this->printMMS();
					}
				}
				else if (method == string("Worst-Fit"))
				{
					if (numAvailable >= it->memorySize)
					{
						if (worstFit_place(it->procID, it->memorySize) == 0)
						{	
							cout << "time " << simuTimer << "ms: Cannot place process " << it->procID << " -- starting defragmentation" << endl;
							defragmentation();
							worstFit_place(it->procID, it->memorySize);
						}
						cout << "time "<< simuTimer <<"ms: Placed process " << it->procID << ":" << endl;
						onRunning.push_back(*it);
						restProcess.erase(it++);
						this->printMMS();
					}
					else
					{
						cout << "time "<< simuTimer <<"ms: Cannot place process " 
						 << it->procID << " -- skipped!" << endl;
						 if(it->listOfJobs.size() > 1){
						 	it->listOfJobs.erase(jobit);
						 }else{
						 	it->listOfJobs.erase(jobit);
						 	finishedProcess.push_back(*it);
						 	restProcess.erase(it++);
						 }
						 this->printMMS();
					}
				}
				else if (method == string("Best-Fit"))
				{
					if (numAvailable >= it->memorySize)
					{
						if (bestFit_place(it->procID, it->memorySize) == 0)
							{	
								cout << "time " << simuTimer << "ms: Cannot place process " << it->procID << " -- starting defragmentation" << endl;
								defragmentation();
								bestFit_place(it->procID, it->memorySize);
							}
							cout << "time "<< simuTimer <<"ms: Placed process " << it->procID << ":" << endl;
							onRunning.push_back(*it);
							restProcess.erase(it++);
							this->printMMS();
					}
					else
					{
						cout << "time "<< simuTimer <<"ms: Cannot place process " 
						 << it->procID << " -- skipped!" << endl;
						 if(it->listOfJobs.size() > 1){
						 	it->listOfJobs.erase(jobit);
						 }else{
						 	it->listOfJobs.erase(jobit);
						 	finishedProcess.push_back(*it);
						 	restProcess.erase(it++);
						 }
						 this->printMMS();
					}
				}
				found = true;
				break;
			}else{
				jobit++;
			}
		}
		if(found == false){//check next if current process is not starting
			it++;
		}
	}
	return found;
}

bool MMS::checkEnd(int t){
	//check all onRunning processes, put any ending process into rest if not finished,
	//into finishedProcess if it's finished
	list<Process>::iterator it = onRunning.begin();
	bool found = false;
	while(it != onRunning.end()){
		bool Yes = false;
		List_of_Jobs::iterator jobit = it->listOfJobs.begin();
		while(jobit != it->listOfJobs.end()){
			if(jobit->second + jobit->first + defragmentationTime <= t){
				if(it->listOfJobs.size() > 1){
					it->listOfJobs.erase(jobit++);
					restProcess.push_back(*it);
				}else if(it->listOfJobs.size() == 1){
					it->listOfJobs.erase(jobit++);
					it->finished = true;
					finishedProcess.push_back(*it);
				}
				if (method == string("noncontiguous"))
				{
					for(int i = 0; i < (int)(it->pageTable.size()); i++)
					{
						mem[it->pageTable[i]] = '.';
						numAvailable += 1;
					}
				}
				else
				{
					for(int i = 0; i < NUM_MEM_FRAME; i++)
					{
						if (mem[i] == it->procID)
						{
							mem[i] = '.';
							numAvailable += 1;
						}
					}
				}
				cout << "time " << t << "ms: Process " << it->procID << " removed:" << endl;
				this->printMMS();
				onRunning.erase(it++);
				found = true;
				Yes = true;
				break;
			}else
				jobit++;
		}
		if(Yes == false){
			it++;
		}
	}
	return found;
}

bool MMS::nonConti_place(vector<int> & pageTable, char procID, int num){
	int availableStart = 0;
	int occupiedStart = 0;
	int count = 0;
	if(numAvailable < num){
		return false;
	}
	while(count < num){//find available frames, update pagetable after placment
		availableStart = findNextAvailable(availableStart);
		occupiedStart = findNextOccupied(availableStart);
		if (availableStart == -1) // End of frames
			break;
		if (occupiedStart < 0)
			occupiedStart = NUM_MEM_FRAME;
		for(int i = availableStart; i < occupiedStart && count < num; i++){
			mem[i] = procID;
			pageTable.push_back(i);
			count++;
			numAvailable -= 1;
		}
	}
	return true;
}

int MMS::nextFit_place(char procID, int num)
{
	bool defragmentation_flag = 0;
	int start = this->cursor;
	int end = this->cursor;
	int temp_cursor = NUM_MEM_FRAME+1;
	while (1)
	{
		if (defragmentation_flag) // after a full loop, no place can be found for specified volume
		{
			cout << "time " << simuTimer << "ms: Cannot place process " << procID << " -- starting defragmentation" << endl;
			defragmentation();
			temp_cursor = NUM_MEM_FRAME;
		}
		start = findNextAvailable(start, temp_cursor);
		if (start == -1)
		{
			temp_cursor = this->cursor;
			start = findNextAvailable(0, temp_cursor);
		}
		if (start == -2)
		{
			defragmentation_flag = 1;
			continue;
		}
		end = findNextOccupied(start, temp_cursor);
		if (end == -1)
		{
			end = NUM_MEM_FRAME-1;
			if (mem[NUM_MEM_FRAME-1] == '.')
				end = NUM_MEM_FRAME;
		}
		if (num <= end-start) //you can place it
		{
			for (int i = start; i<start+num; i++)
			{
				this->mem[i]=procID;
				numAvailable--;
			}
			this->cursor = start + num;
			return start;
		}
		else
		{
			start = end+1;
			continue;
		}
	}

}

bool MMS::worstFit_place(char procID, int num){
	map<int, vector<int> > availableBlocks;
	int availableStart = 0;
	int occupiedStart = 0;
	if(numAvailable < num){
		return false;
	}
	//collect all available sections of frames, save their sizes as keys in map
	//and save their starting position as associated value in map
	while(1){
		availableStart = findNextAvailable(availableStart);
		if (availableStart == -1) // End of frames
			break;
		occupiedStart = findNextOccupied(availableStart);
		if (occupiedStart == -1)
		{
			if (mem[NUM_MEM_FRAME-1] == '.')
				occupiedStart = NUM_MEM_FRAME;
			else
				occupiedStart = NUM_MEM_FRAME - 1;
		}
			
		int available_frames = occupiedStart - availableStart;
		availableBlocks[available_frames].push_back(availableStart);
		availableStart = occupiedStart;
	}
	//the last element stores information of section we want to find
	map<int , vector<int> >::iterator it = --availableBlocks.end();
	int availablesize = it->first;
	if(availablesize >= num){//if this section is large enough
		int start = it->second[0];
		for(int i = 0; i < num; i++){
			mem[start + i] = procID;
			numAvailable -= 1;
		}
		return true;
	}
	else
	{//if this section is too small, return false
		return false;
	}
}

bool MMS::bestFit_place(char procID, int num){
	map<int, vector<int> > availableBlocks;
	int availableStart = 0;
	int occupiedStart = 0;
	if(numAvailable < num){
		return false;
	}
	while(1){//collect all available sections
		availableStart = findNextAvailable(availableStart);
		if (availableStart == -1) 
			break;
		occupiedStart = findNextOccupied(availableStart);
		if (occupiedStart == -1)
		{
			if (mem[NUM_MEM_FRAME-1] == '.')
				occupiedStart = NUM_MEM_FRAME;
			else
				occupiedStart = NUM_MEM_FRAME - 1;
		}	
		int available_frames = occupiedStart - availableStart;
		availableBlocks[available_frames].push_back(availableStart);
		availableStart = occupiedStart;
	}
	//the first element stores information of section we want to find
	map<int , vector<int> >::iterator it = availableBlocks.begin();
	bool canPlace = false;
	for(; it != availableBlocks.end(); it++){
		int availablesize = it->first;
		if(availablesize >= num){
			canPlace = true;
			break;
		}
	}
	if(canPlace){//the section is large enough
		int start = it->second[0];
		for(int i = 0; i < num; i++){
			mem[start + i] = procID;
			numAvailable -= 1;
		}
		return true;
	}else
	{//not large enough
		return false;
	}
}

//-------------Virtual Memory--------------------//
void simulation(const vector<int> & pages, const string & method); 
int futureForward(const vector<int> & pages, int current, vector<int>& mem);
int RecentBackward(const vector<int> & pages, int current, vector<int>& mem);
int FrequentBackward(map<int, int> & forLFU, const vector<int>& pages, vector<int>& mem);

void virtual_memory(const vector<int> & pages){
	simulation(pages, "OPT");
	cout << endl;
	simulation(pages, "LRU");
	cout << endl;
	simulation(pages, "LFU");
}

void simulation(const vector<int> & pages, const string& method){
	int numOfVictim = 0;
	vector<int> mem(3,0);
	cout << "Simulating " << method << " with fixed frame size of 3" << endl;
	//this map is used to record reference count of pages in the past
	map<int, int> forLFU;
	for(int i = 0; i < (int) (pages.size()); i++){
		bool hasvictim = true;//true if there is victim page
		bool newitem = false;//true if unused memory is being used
		for(int j = 0; j < 3; j++){
		//if current page is already in memory, jump to next
			if(mem[j] == pages[i]){
				hasvictim = false;
				break;
			}
		//if there is empty slot, put current page in that slot
			if(mem[j] == 0){
				mem[j] = pages[i];
				newitem = true;
				break;
			}
		}
		if(hasvictim){//if there is victim page
			numOfVictim++;
			int victimpage = 0;
			if(!newitem && method == "OPT"){
			//get the index of victim page in memory, record and replace it
				int index = futureForward(pages, i, mem);
				victimpage = mem[index];
				mem[index] = pages[i];
			}
			if(!newitem && method == "LRU"){
			//similar as above
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
				//reset the reference count, since it will be incremented later, 
				//reset it to be 0
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
		forLFU[pages[i]]++;//increment reference count
	}
	cout << "End of " << method <<  " simulation ("  << numOfVictim << " page faults)" << endl;
}
int futureForward(const vector<int> & pages, int current, vector<int>& mem){
	//each pair in this vector store whether corresponding page number in 
	//memory will be access in the future, and the how far it is if so.
	vector<pair<bool, int> > index(3, make_pair(false, 0));
	int count = 0;
	int victimIndex = 0;
	//from current position, search forward
	for(int i = current + 1; i < (int)(pages.size());i++){
		for(int j = 0; j < 3; j++){
			//only record the position of first access in the future
			if(mem[j] == pages[i] && index[j].first == false){
				index[j].first = true;
				index[j].second = i;
				count++;
			}
		}
		if(count == 3){//break when all 3 pages in memory are found in future
			break;
		}
		
	}
	int max = 0;
	//this map is used to return lowest numbered page if there is a tie
	map<int , int> noInFuture;
	for(int i = 0; i < 3; i++){
		//get farthest position of future access of pages in memory
		if(index[i].second > max){
			max = index[i].second;
			victimIndex = i;
		}
		//if page will not be accessed in future, add it to map
		if(index[i].first == false){
			noInFuture[mem[i]] = i; 
		}
	}
	if(noInFuture.size() > 0){
		//the first element in map is the lowest numbered page 
		return noInFuture.begin()->second;
	}
	return victimIndex;
}

int RecentBackward(const vector<int> & pages, int current, vector<int>& mem){
	//similar to OPT
	vector<pair<int, bool> > index(3, make_pair(0, false));
	int count = 0;
	int victimIndex = 0;
	bool found = false;
	int min = pages.size();
	//from current position, search backward
	for(int i = current - 1 ; i >= 0;i--){
		for(int j = 0; j < 3; j++){
			if(mem[j] == pages[i] && index[j].second == false){
				index[j].second = true;
				index[j].first = i;
				if(min > i){//record oldest accessed page
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
			//if there is a tie, record the lower numbered page
			if(min == forLFU[mem[i]]){
				index = mem[i] > mem[index] ? index:i;
			}else{//record page who has reference count
				min = forLFU[mem[i]];
				index = i;
			}
		}
	}
	return index;
}


int main(int argc, char* argv[])
{
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

	MMS mms1(numOfProcess, allProcesses);
	MMS mms2(numOfProcess, allProcesses);
	MMS mms3(numOfProcess, allProcesses);
	MMS mms4(numOfProcess, allProcesses);

	mms1.method = "Next-Fit";
	mms1.contiguous();
	cout << endl;

	mms2.method = "Best-Fit";
	mms2.contiguous();
	cout << endl;

	mms3.method = "Worst-Fit";
	mms3.contiguous();
	cout << endl;

	mms4.nonContiguous();
	cout << endl;

	//process virtual memeory input file, save all page numbers 
	ifstream vm(argv[2]);
	int page_num;
	string page;
	vector<int> pages;
	while(getline(vm, line)){
		istringstream ss(line);
		while(getline(ss, page, ' ')){
			page_num = atoi(page.c_str());
			pages.push_back(page_num);
		}
		
	}
	virtual_memory(pages);
	return 0;
}