/* Memory.h */

#include <iostream>
#include <map>
#include <assert.h>
#include <vector>

using namespace std;

/* ==================== Header and functions for Memory class ======================*/
/* Memory class header  */
typedef map<int, int> mem_map;

class Memory {
public:
    /* public varables  */
    vector<char> mem;
    int num_used;
    int num_free;
    int last_index;       /* last used index          */
    mem_map index_map;    /* map with index as key    */
    mem_map space_map;    /* map with space as map    */
    map<int, pair<char, int> > char_map;
    
    /* constructors */
    Memory();
    
    /* member functions */
    bool allocate(const char& p_id, const int& num_space, const string& alg);
    void set_partition(const int& s_index, const int& num_space, const string& alg);
    void deallocate(const char& p_id, const int& num_space, const string& alg);
    int defragmentation(vector<char>& removed_chars);
    void reset();
    void print();
    void print_map();
    void print_s_map();
    void print_char_map();
};

/* default constructor for Memory class */
Memory::Memory() {
    /* allocate a martix of char '.' with size 8*32 */
    for (unsigned int i = 0; i < 256; ++i) {
        mem.push_back('.');
    }

    last_index = 0;
    num_used = 0;
    num_free = 256;
    index_map.insert(make_pair(0,256));
    space_map.insert(make_pair(256,0));
}

/* allocate memeories   */
bool Memory::allocate(const char& p_id, const int& num_space, const string& alg) {
    if (alg == "Non-contiguous") {
        // loop index map to allocate memory
        mem_map::iterator itr = index_map.begin();
        int num_space_left = num_space;
        
        // continuously occupy the memory from the first start_index 
        // until all space needed are occupied
        for (; itr != index_map.end() && num_space_left != 0;) {
            
            int tmp_occupy = min(num_space_left, itr->second);
            unsigned int index = itr->first; // start point of a placement
            for (int i = 0; i < tmp_occupy; ++i) {
                mem[index] = p_id;
                ++index;
            }
            // if the space needed is larger than current interval
            if (tmp_occupy == itr->second) { 
                char_map.insert(make_pair(itr->first, make_pair(p_id, itr->second)));
            } else { // if the space needed is smaller than current interval
                index_map.insert(make_pair(itr->first + tmp_occupy, itr->second - tmp_occupy));
                char_map.insert(make_pair(itr->first, make_pair(p_id, num_space_left)));
            }
            index_map.erase(itr++);
            num_space_left -= tmp_occupy;
        }
        
        num_used += num_space;
        num_free -= num_space;
        return (num_space_left == 0);
    }
    
    if (alg == "First-Fit") {
        /* loop index map to allocate memory */
        for (mem_map::iterator itr = index_map.begin(); itr != index_map.end(); ++itr) {
            if (itr->second >= num_space) {
                unsigned int index = itr->first;
                for (int i = 0; i < num_space; ++i) {
                    mem[index] = p_id;
                    ++index;
                }
                num_used += num_space;
                num_free -= num_space;
                index_map.insert(make_pair(itr->first + num_space, itr->second - num_space));
                char_map.insert(make_pair(itr->first, make_pair(p_id, num_space)));
                index_map.erase(itr);
                return true;
            }
        }
        return false;
    }
    
    if (alg == "Next-Fit") {

        // first, find the empty block that covers where the newly removed process was
        mem_map::iterator itr = index_map.begin();
        for(; itr != index_map.end(); ++itr) {
            if (itr->first + itr->second >= last_index) 
                break;
        }
        mem_map::iterator flag = itr; // record the place of last updated
        // try placing the incoming process in this empty block.
        // If can, place it 

        if(itr->first + itr->second >= last_index + num_space) {
            // update the memory graph
            for(int i = last_index; i < last_index + num_space; ++i) {
                mem[i] = p_id;
            }

            int tmp_index = itr->first;
            int tmp_num_space = itr->second;
            index_map.erase(itr);

            // update index_map, char_map
            if (tmp_index != last_index) {
                index_map.insert(make_pair(tmp_index, tmp_num_space - num_space));
                char_map.insert(make_pair(last_index, make_pair(p_id, num_space)));
            } else {
                index_map.insert(make_pair(tmp_index + num_space, tmp_num_space - num_space));
                char_map.insert(make_pair(tmp_index, make_pair(p_id, num_space)));
            }
            // update num_used, num_free, last_index
            num_used += num_space;
            num_free -= num_space;
            last_index += num_space;
            return true;

        } else { //if cannot, loop to the bottom of the memory map, and then reloop from the top

            flag++;

            do { // if there is enough place from the last index to the bottom
                if (itr->second >= num_space) {
                    unsigned int index = itr->first;
                    for (int i = 0; i < num_space; ++i) {
                        mem[index] = p_id;
                        ++index;
                    }
                    num_used += num_space;
                    num_free -= num_space;
                    index_map.insert(make_pair(itr->first + num_space, itr->second - num_space));
                    char_map.insert(make_pair(itr->first, make_pair(p_id, num_space)));
                    index_map.erase(itr++);
                    return true;
                    
                } else itr++;
                // if arrive the tail, return to the beginning
                if (itr == index_map.end()) itr = index_map.begin();

            } while (itr != flag);
        }
        return false;
    }
    
    if (alg == "Best-Fit") {
        /* loop space map to allocate memory    */
        for (mem_map::iterator itr = space_map.begin(); itr != space_map.end(); ++itr) {
            if (itr->first >= num_space) {
                unsigned int index = itr->second;
                for (int i = 0; i < num_space; ++i) {
                    mem[index] = p_id;
                    ++index;
                }
                num_used += num_space;
                num_free -= num_space;
                if (itr->first - num_space != 0) {
                    space_map.insert(make_pair(itr->first - num_space, itr->second + num_space));
                }
                char_map.insert(make_pair(itr->second, make_pair(p_id, num_space)));
                space_map.erase(itr);
                return true;
            }
        }
        return false;
    }
    
    return false;
}

/* set up partition */
void Memory::set_partition(const int& s_index, const int& num_space, const string& alg) {
    mem_map* map_p;
    if (alg == "First-Fit" || alg == "Next-Fit"|| alg == "Non-contiguous") map_p = &index_map;
    if (alg == "Best-Fit") map_p = &space_map;
    
    bool merge_left = false;
    bool merge_right = false;
    int front_i, front_space, end_space;
    if (s_index - 1 >= 0 && mem[s_index - 1] == '.') {
        merge_left = true;
        for (mem_map::iterator itr = (*map_p).begin(); itr != (*map_p).end(); ++itr) {
            int map_index, map_space;
            if (alg == "First-Fit" || alg == "Next-Fit"|| alg == "Non-contiguous") {
                map_index = itr->first;
                map_space = itr->second;
            }
            if (alg == "Best-Fit") {
                map_index = itr->second;
                map_space = itr->first;
            }
            if (map_index + map_space == s_index) {
                front_i = map_index;
                front_space = map_space;
                map_p->erase(itr);
                break;
            }
        }
    }
    // if (s_index + num_space + 1 < 256 && mem[s_index + num_space + 1] == '.') {
    if (s_index + num_space < 256 && mem[s_index + num_space] == '.') {
        merge_right = true;
        for (mem_map::iterator itr = (*map_p).begin(); itr != (*map_p).end(); ++itr) {
            int map_index, map_space;
            if (alg == "First-Fit" || alg == "Next-Fit"|| alg == "Non-contiguous") {
                map_index = itr->first;
                map_space = itr->second;
            }
            if (alg == "Best-Fit") {
                map_index = itr->second;
                map_space = itr->first;
            }
            if (map_index == s_index + num_space) {
                end_space = map_space;
                map_p->erase(itr);
                break;
            }
        }
    }
    int new_index;
    int new_space;
    if (merge_left && merge_right) {
        new_index = front_i;
        new_space = front_space + num_space + end_space;
    }
    else if (merge_left) {
        new_index = front_i;
        new_space = front_space + num_space;
    }
    else if (merge_right) {
        new_index = s_index;
        new_space = num_space + end_space;
    }
    else {
        new_index = s_index;
        new_space = num_space;
    }
    if (alg == "First-Fit" || alg == "Next-Fit"|| alg == "Non-contiguous") 
        map_p->insert(make_pair(new_index, new_space));
    else if (alg == "Best-Fit") map_p->insert(make_pair(new_space, new_index));
}

/* deallocate memories  */
void Memory::deallocate(const char& p_id, const int& num_space, const string& alg) {
    
    // loop the char map to find the start index the process and deallocate
    map<int, pair<char, int> >::iterator itr = char_map.begin();

    for (; itr != char_map.end();) {
        if (itr->second.first == p_id) {
            int s_index = itr->first;
            
            // change the related symbol from p_id to '.'
            for (int i = s_index; i < s_index + itr->second.second; ++i) {
                assert(mem[i] == p_id);
                mem[i] = '.';
            }
            /* update index_map or space_map and combine partitions if necessary  */
            // update the last_index
            if(alg == "Next-Fit") last_index = s_index + itr->second.second;
            set_partition(s_index, itr->second.second, alg);
            char_map.erase(itr++);
        }
        else itr++;
    }
    num_used -= num_space;
    num_free += num_space;
    
}

/* defragmentation  */
int Memory::defragmentation(vector<char> &removed_chars) {
    map<int, pair<char, int> >::iterator itr = char_map.begin();
    int index = 0, num_removed = 0;
    map<int, pair<char, int> > new_map;
    // insert new status to new_map and move the symbol
    for (; itr != char_map.end(); ++itr) {
        new_map.insert(make_pair(index, itr->second));
        for (int i = 0; i < itr->second.second; ++i) {
            mem[index] = itr->second.first;
            ++index;
        }
        // check if the first block should be moved
        if (itr->first != 0) {
            num_removed += itr->second.second; // if the first block should be moved
            removed_chars.push_back(itr->second.first);
        }
    }
    
    // reset the rest part of mem diagram
    char_map.clear();
    char_map = new_map;
    map<int, pair<char, int> >::iterator tmp_itr = char_map.end();
    tmp_itr--;
    int left_s_index = tmp_itr->first + tmp_itr->second.second;
    for (int i = left_s_index; i < 256; ++i) {
        mem[i] = '.';
    }
    
    index_map.clear();
    space_map.clear();
    index_map.insert(make_pair(num_used, num_free));
    space_map.insert(make_pair(num_free, num_used));
    
    return num_removed;
}

/* print out memory */
void Memory::print() {
    for (unsigned int i = 0; i < 32; ++i) cout << "=";
    cout << endl;
    unsigned int index = 0;
    for (unsigned int i = 0; i < 8; ++i) {
        for (unsigned int j = 0; j < 32; ++j) {
            cout << mem[index];
            ++index;
        }
        cout << endl;
    }
    for (unsigned int i = 0; i < 32; ++i) cout << "=";
    cout << endl;
}
/* reset Memory */
void Memory::reset() {
    mem.clear();
    index_map.clear();
    space_map.clear();
    char_map.clear();
    
    /* allocate a martix of char '.' with size 8*32 */
    for (unsigned int i = 0; i < 256; ++i) {
        mem.push_back('.');
    }
    last_index = 0;
    num_used = 0;
    num_free = 256;
    index_map.insert(make_pair(0,256));
    space_map.insert(make_pair(256,0));
}

/* helper functions for debugging*/
/* print index map */    
void Memory::print_map() {
    for (mem_map::iterator itr = index_map.begin(); itr != index_map.end(); ++itr) {
        cout << itr->first << " " << itr->second << endl;
    }
}

/* print space map */
void Memory::print_s_map() {
    for (mem_map::iterator itr = space_map.begin(); itr != space_map.end(); ++itr) {
        cout << itr->first << " " << itr->second << endl;
    }
}
/* print char map */
void Memory::print_char_map() {
    for (map<int, pair<char, int> >::iterator itr = char_map.begin(); 
        itr != char_map.end(); ++itr) {
        cout << "<" << itr->first << " " << "<" << itr->second.first << " , " 
             << itr->second.second << "> >" << endl;
    }
}