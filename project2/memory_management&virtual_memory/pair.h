//
//  pair.h
//  OpSys_p3
//
//  Created by Chelsey on 5/2/16.
//  Copyright (c) 2016 Chelsey. All rights reserved.
//

#ifndef OpSys_p3_pair_h
#define OpSys_p3_pair_h

class Process {
public:
    char p_id;
    unsigned int p_mem;
    Process(char symbol, unsigned int mem): p_id(symbol), p_mem(mem){}
};

class Proc_pair {
public:
    unsigned int time;
    Process proc;
    Proc_pair(unsigned int t, Process p): time(t), proc(p){}
};

struct Comparer{
    bool operator()(const Proc_pair& pair1, const Proc_pair& pair2) const {
        return pair1.time < pair2.time ||
        (pair1.time == pair2.time && pair1.proc.p_id < pair2.proc.p_id);
    }
};



#endif
