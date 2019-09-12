#include <string>
using namespace std;
struct ident
{
	int level;
	int position;
};

struct Ipv4RoutingTableEntry
{
    string destAddress;
	string nextAddress;
	bool flag;
};