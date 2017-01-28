// Virtual Memory & Memory management 
/* Compile as g++ main.cpp 
   Run as ./a.out test.txt virtual_memory_input2.txt 
   Given input file name" test.txt */


//Teammates: Bing Yang, Hao Chen, Xu Chai.

#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <string>
#include <map>
#include <stdio.h>
#include <stdlib.h> 
#include <assert.h>
#include "pair.h"
#include "memory.h"
#include <vector>
#include "virtual_memory.h"
#include "stdlib.h"
#include <climits>
#include <list>
using namespace std;

# define t_memmove 1 /* time of moving each unit in defragmentation  */

/* <[time_value, (process_id, memory units needed)], event_type> */
typedef map<Proc_pair, int, Comparer> my_map; 
int final_t;

/* helper function for debugging */
void print_event_map(const my_map& event_map) {
    for (my_map::const_iterator itr = event_map.begin(); itr != event_map.end(); ++itr) {
        cout << " | " << itr->first.time << " " 
            << itr->first.proc.p_id << " " << itr->first.proc.p_mem
            << " | " << itr->second << " | " << endl;
        cout << "-------------" << endl;
    }
}

/* Useful helper functions. */
bool is_digit(const string& str) {
    for(unsigned int i = 0; i < str.size(); ++i) {
        if (!isdigit(str[i])) 
            return false;
    }
    return true;
}

void error_handler(){
    cout << "Error: Invalid input content!" << endl;
    exit(1);
}

void skip_blanks(unsigned int& i, const string& str) {

    for (; i < str.size(); ++i) {
        if(str[i] == ' ' || str[i] == '\t') 
            continue;
        else 
            break;
    }
}

/* parse each line of input and build the event_map */
void parse_a_line(string line, my_map& event_map){
    
    // get the process_id
    char symbol = line[0];
    if (!isupper(symbol)) {
        error_handler();
    }
    // check the uniqueness of the process id
    map<char, int> p_id_map;
    pair< map<char, int>::iterator, bool> check_dup;
    check_dup= p_id_map.insert(make_pair(symbol, 1));
    if (check_dup.second == false) {
        error_handler();
    }

    // get the number of memory required of each process and make a process object
    unsigned int i = 1;
    skip_blanks(i, line);

    string mem_num = "";
    while (line[i] != ' ') mem_num += line[i++];
    if (!is_digit(mem_num)) {
        error_handler();
    }
    Process tmp_p(symbol, atoi(mem_num.c_str()));

    // continue parse the following time interval part
    skip_blanks(i, line);
    string tmp = "", start, end;
    for (; i < line.size(); ++i) {
        if (line[i] == '-') {
            start = tmp;
            // make a pair and insert to event_map
            Proc_pair tmp_key(atoi(start.c_str()), tmp_p);
            // 0 means start using memory, 1 means finish using memory
            event_map.insert(make_pair(tmp_key, 0)); 
            tmp = "";
        } else if (line[i] == ' ' || line[i] == '\t') {
            if(line[i + 1] == ' ' || line[i + 1] == '\t') continue;
            else {
                end = tmp;
                // make a pair and insert to event_map
                Proc_pair tmp_key(atoi(end.c_str()), tmp_p);
                event_map.insert(make_pair(tmp_key, 1));
                tmp = "";
            }
        } else {
            tmp += line[i];
            if (i == line.size() - 1) {
                end = tmp;
                // make a pair and insert to event_map
                Proc_pair tmp_key(atoi(end.c_str()), tmp_p);
                event_map.insert(make_pair(tmp_key, 1));
            }
        }
    }
}

/* Read in input file*/
void read_file(ifstream &in_str, my_map& event_map) {
    string line;
    unsigned int num;
    getline(in_str, line);
    // check validity of the first line of input
    if (!is_digit(line)) {
        error_handler();
    }
    num = atoi(line.c_str());
    if (num > 26) {
        error_handler();
    }
    for (; num > 0 && !in_str.eof(); --num){
        getline(in_str, line);
        parse_a_line(line, event_map);
    }
}

/* simulate each algorithm */
void start_simulate(my_map event_map, Memory memory, const string& alg) {
    // reset
    memory.reset();
    final_t = 0;
    bool defrag = false;

    cout << "time 0ms: Simulator started (";
    if(alg != "Non-contiguous") cout << "Contiguous -- ";
    cout << alg << ")" << endl;
    
    for (my_map::iterator itr = event_map.begin(); itr != event_map.end(); ++itr) {
        // if event_map is switched, return the itr to event_map.begin()
        if(defrag == true) {
            itr--;
            defrag = false;
        }
        // Process p = itr->first.proc;
        int tmp_t = itr->first.time;
        final_t = tmp_t;
        char p_id = itr->first.proc.p_id;
        int mem_num = itr->first.proc.p_mem;
        int event_type= itr->second;
        
  
        // if event type is 0, allocate memory
        if (event_type == 0) { 

            cout << "time " << tmp_t << "ms: Process " << p_id << " arrived " <<
                    "(requires " << mem_num << " frames of physical memory)" << endl;

            // If free memory units are not enough (num_free < mem_num): skipping:  
            if (memory.num_free < mem_num) {
                cout << "time " << tmp_t << "ms: Cannot place process " << p_id << 
                    " -- skipping process " << p_id << endl;  
                memory.print();

                // Erase this process record from event_map 
                // (including both arriving and leaving one):
                my_map::iterator tmp_itr = itr;
                tmp_itr++;
                for (; tmp_itr != event_map.end();) {
                    char tmp_id = tmp_itr->first.proc.p_id;
                    if (tmp_id == p_id) event_map.erase(tmp_itr++);
                    else tmp_itr++;
                }
            } 
            // If free memory units are enough (num_free >= mem_num): try allocating
            else { 

                // if cannot allocate directly, start defragmentation
                if (memory.allocate(p_id, mem_num, alg) == false) {

                    cout << "time " << tmp_t << "ms: Cannot place process " << p_id << 
                        " -- starting defragmentation" << endl;

                    vector<char>removed_chars;
                    int num_move = memory.defragmentation(removed_chars);
                    int time_move = num_move * t_memmove;
                    int new_time = tmp_t + num_move * t_memmove;;
                    
                    cout << "time " << new_time << "ms: Defragmentation complete (moved " 
                        << num_move << " frames:";
                    for (unsigned int i = 0; i < removed_chars.size() - 1; ++i) 
                        cout << " "<< removed_chars[i] << ",";
                    cout << " " << removed_chars[removed_chars.size() - 1] << ")" << endl;
                    memory.print();

                    // place the process which triggered the defragmentation
                    cout << "time " << new_time << "ms: Placed process " << p_id 
                         << " in memory:" << endl;
                    memory.allocate(p_id, mem_num, alg);
                    memory.print();

                    /* for those 'suspending' events, add defragmentation time to related events */
                    my_map::iterator tmp_itr = itr;
                    tmp_itr++;
                    my_map new_event_map;

                    for (; tmp_itr != event_map.end(); ++tmp_itr) {
                        int new_t = tmp_itr->first.time + time_move;
                        Proc_pair tmp_key(new_t, tmp_itr->first.proc);
                        new_event_map.insert(make_pair(tmp_key, tmp_itr->second));
                    }
                    event_map = new_event_map;
                    defrag = true;
                    itr = event_map.begin();
                } 
                // if allocate successfully
                else { 
                    cout << "time " << tmp_t << "ms: Placed process " << p_id 
                         << " in memory:" << endl;
                    memory.print();
                }
            }
        } 
        // if event type is 1, remove memory
        else { 
            cout << "time " << tmp_t << "ms: Process " << p_id 
                 << " removed from physical memory" << endl;
            memory.deallocate(p_id, mem_num, alg);
            memory.print();
        }
    }
    cout << "time " << final_t << "ms: Simulator ended (";
    if(alg != "Non-contiguous") cout << "Contiguous -- ";
    cout << alg << ")\n" << endl;
}
//===============================Virtual Memory============================
void read_file_2(ifstream &in_str, vector<int> &page_reference, 
                 map<int,list<int> > &page_index)
{
    int count = 0;
    string tmp;
    while(in_str >> tmp)
    {
        bool wrong_input = false;
        for(unsigned int i = 0 ; i < tmp.size(); i++)
        {
            if(!isdigit(tmp[i]))
            {
                wrong_input = true;
                cerr<<"Wrong_input: "<<tmp<<endl;
                break;
            }
        }
        if(!wrong_input)
        {
            int a = atoi(tmp.c_str());
            page_reference.push_back(a);
            page_index[a].push_back(count);
            count++;
        }
        else
        {
            break;
        }
    }
}

void print_frame( int frame[])
{
    cout<<"[mem: ";
    for(int i = 0 ; i < F;i++)
    {
        if(frame[i] >= 0 && i != (F-1))
        {
            cout<<frame[i]<<" ";
        }
        else if(frame[i] >= 0)
        {
            cout<<frame[i];
        }
        else if(i == (F-1))
        {
            cout<<".";
        }
        else
        {
            cout<<". ";
        }
    }
    cout<<"]";
}

//----------------------------OPT replacement algo----------------------------
void OPT(vector<int> page_reference, map<int,list<int> > page_index, 
         Virtual_memory virtual_memory)
{
    int count_fault = 0;
    cout<<"Simulating OPT with fixed frame size of "<<F<<endl;
    for(unsigned int i = 0; i < page_reference.size();i++)
    {
        bool is_find = false;
        for(int l = 0 ; l < F;l++)
        {
            if(page_reference[i] == virtual_memory.frame[l])
            {
                is_find = true;
                break;
            }
        }
        if( !is_find)
        {
            if(virtual_memory.get_left_unused() > 0)
            {
                    int j = 0;
                    while(virtual_memory.frame[j] >= 0)
                    {
                        j++;
                    }
                    virtual_memory.frame[j] = page_reference[i];
                    page_index[page_reference[i]].pop_front();
                    int tmp = virtual_memory.get_left_unused();
                    virtual_memory.set_left_unused(--tmp);
                    cout<<"referencing page "<<page_reference[i]<<" ";
                    print_frame(virtual_memory.frame);
                    cout<<" PAGE FAULT (no victim page)"<<endl;
                    count_fault++;
            }
            else
            {
                int replace_page = -1;
                int replace_page_index = -2;
                int replace_page_frame_index = -1;
                for(int m = 0 ; m < F; m++)
                {
                    int tmp = -1;
                    list<int>::iterator itr = page_index[virtual_memory.frame[m]].begin();
                    if(itr != page_index[virtual_memory.frame[m]].end())
                    {
                        tmp = *itr;
                    }
                    if(tmp != -1 && replace_page_index != -1)
                    {
                        if(tmp > replace_page_index )
                        {
                            replace_page = virtual_memory.frame[m];
                            replace_page_frame_index = m;
                            replace_page_index = tmp;
                        }
                    }
                    else if(tmp == -1)
                    {
                        if(replace_page_index == -1)
                        {
                            if(replace_page > virtual_memory.frame[m])
                            {
                                replace_page = virtual_memory.frame[m];
                                replace_page_frame_index = m;
                                replace_page_index = tmp;
                            }
                        }
                        else
                        {
                            replace_page = virtual_memory.frame[m];
                            replace_page_frame_index = m;
                            replace_page_index = tmp;
                        }
                    }
                }
                int victim_page = virtual_memory.frame[replace_page_frame_index];
                virtual_memory.frame[replace_page_frame_index] = page_reference[i];
                page_index[page_reference[i]].pop_front();
                cout<<"referencing page "<<page_reference[i]<<" ";
                print_frame(virtual_memory.frame);
                cout<<" PAGE FAULT (victim page "<<victim_page<<")"<<endl;
                count_fault++;

            }
        }
        else
        {
            cout<<"referencing page "<<page_reference[i]<<" ";
            print_frame(virtual_memory.frame);
            cout<<endl;
            page_index[page_reference[i]].pop_front();
        }
    }
    cout<<"End of OPT simulation ("<<count_fault<<" page faults)"<<endl;
    cout<<endl;
}
//---------------------------LRU replacement algo----------------------------
void LRU(vector<int> page_reference, Virtual_memory virtual_memory)
{

    int count_fault = 0;
    cout<<"Simulating LRU with fixed frame size of "<<F<<endl;
    for(unsigned int i = 0; i < page_reference.size();i++)
    {
        bool is_find = false;
        for(int l = 0 ; l < F;l++)
        {
            if(page_reference[i] == virtual_memory.frame[l])
            {
                is_find = true;
                break;
            }
        }
        if(!is_find)
        {
            //Frame is not full of pages
            if(virtual_memory.get_left_unused() > 0)
            {
                int j = 0;
                while(virtual_memory.frame[j] >= 0)
                {
                    j++;
                }
                virtual_memory.frame[j] = page_reference[i];
                int tmp = virtual_memory.get_left_unused();
                virtual_memory.set_left_unused(--tmp);
                cout<<"referencing page "<<page_reference[i]<<" ";
                print_frame(virtual_memory.frame);
                cout<<" PAGE FAULT (no victim page)"<<endl;
                count_fault++;
            }
            else
            {
                map<int,int> backward_count;
                int least_recently_page;
                int k = i-1;
                while(backward_count.size() < F && k >= 0)
                {
                    backward_count[page_reference[k]]++;
                    least_recently_page = page_reference[k];
                    k--;
                }
                int least_recently_page_index;
                for(int n = 0 ; n < F ; n++)
                {
                    if(least_recently_page == virtual_memory.frame[n])
                        least_recently_page_index = n;
                }
                virtual_memory.frame[least_recently_page_index] = page_reference[i];
                cout<<"referencing page "<<page_reference[i]<<" ";
                print_frame(virtual_memory.frame);
                cout<<" PAGE FAULT (victim page "<<least_recently_page<<")"<<endl;
                count_fault++;
            }
        }
        //This page has existed in the frame.
        else
        {
            cout<<"referencing page "<<page_reference[i]<<" ";
            print_frame(virtual_memory.frame);
            cout<<endl;
        }
    }
    cout<<"End of LRU simulation ("<<count_fault<<" page faults)"<<endl;
    cout<<endl;
}
//-----------------------------LFU replacement algo----------------------------
void LFU(vector<int> page_reference, Virtual_memory virtual_memory)
{
    int count_fault = 0;
    cout<<"Simulating LFU with fixed frame size of 3"<<endl;
    map<int, int> page_times;
    for(unsigned int i = 0; i < page_reference.size();i++)
    {
        bool is_find = false;
        for(int l = 0 ; l < F;l++)
        {
            if(page_reference[i] == virtual_memory.frame[l])
            {
                is_find = true;
                break;
            }
        }
        if(!is_find)
        {
            //Frame is not full of pages
            if(virtual_memory.get_left_unused() > 0)
            {
                int j = 0;
                while(virtual_memory.frame[j] > 0)
                {
                    j++;
                }
                virtual_memory.frame[j] = page_reference[i];
                int tmp = virtual_memory.get_left_unused();
                virtual_memory.set_left_unused(--tmp);
                cout<<"referencing page "<<page_reference[i]<<" ";
                print_frame(virtual_memory.frame);
                cout<<" PAGE FAULT (no victim page)"<<endl;
                count_fault++;
                page_times[page_reference[i]] = 1;
            }
            else
            {

                int replace_page = INT_MAX;
                int replace_page_index = INT_MAX;
                int replace_page_times = INT_MAX;
                for(int q = 0 ; q < F; q++)
                {
                    if(page_times[virtual_memory.frame[q]] < replace_page_times )
                    {
                        replace_page = virtual_memory.frame[q];
                        replace_page_times = page_times[replace_page];
                        replace_page_index = q;
                    }
                    else if(page_times[virtual_memory.frame[q]] == replace_page_times 
                            && virtual_memory.frame[q] < replace_page)
                    {
                        replace_page = virtual_memory.frame[q];
                        replace_page_times = page_times[replace_page];
                        replace_page_index = q;
                    }
                }
                virtual_memory.frame[replace_page_index] = page_reference[i];
                cout<<"referencing page "<<page_reference[i]<<" ";
                print_frame(virtual_memory.frame);
                cout<<" PAGE FAULT (victim page "<<replace_page<<")"<<endl;
                count_fault++;
                page_times[page_reference[i]] = 1;
            }
        }
        //This page has existed in the frame.
        else
        {
            cout<<"referencing page "<<page_reference[i]<<" ";
            page_times[page_reference[i]]++;
            print_frame(virtual_memory.frame);
            cout<<endl;
        }
    }
    cout<<"End of LFU simulation ("<<count_fault<<" page faults)"<<endl;
}
//=========================Virtual Memory=======================


int main(int argc, const char * argv[]) {
    if(argc != 3)
    {
        cerr<<"Using arguments: arg1 arg2 arg3"<<endl;
    }
    my_map event_map; 
    ifstream in_str(argv[1]);
    if (!in_str.good()) {
        cerr << "Can't open " << argv[1] << "to read.\n";
        exit(1);
    }

    read_file(in_str, event_map);
    // print_event_map(event_map);

    Memory memory;
    // Start simulation
    start_simulate(event_map, memory, "First-Fit");
    start_simulate(event_map, memory, "Next-Fit");
    start_simulate(event_map, memory, "Best-Fit");
    start_simulate(event_map, memory, "Non-contiguous");

    //======================Virtual Memory=========================
    ifstream in_str_2(argv[2]);
    if(!in_str_2.good())
    {
        cerr << "Can't open " << argv[2] << "to read.\n";
        exit(1);
    }
    map<int, list<int> > page_index;
    vector<int> page_reference;
    read_file_2(in_str_2,page_reference,page_index);
    Virtual_memory virtual_memory;
    OPT(page_reference,page_index,virtual_memory);
    LRU(page_reference,virtual_memory);
    LFU(page_reference,virtual_memory);
    return 0;
}