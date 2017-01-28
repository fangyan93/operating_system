/* virtual_memory.h */
#include <vector>
#include <map>

#define F 3
using namespace std;
class Virtual_memory
{
public:
	Virtual_memory()
	{
		left_unused = F;
		for(int i = 0 ; i < F ; i++)
		{
			frame[i] = -1;
		}
	}
	int get_left_unused(){return left_unused;}
	void set_left_unused(int a ){left_unused = a;}
	int frame[F];
private:
	int left_unused;//left unused frame
	
};