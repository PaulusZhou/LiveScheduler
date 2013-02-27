#include <iostream>
#include <set>
using namespace std;
struct info{
    float cpu;
    float concurrecy;
    float response;
    float netrates;
};
typedef set<struct info> Set;
typedef Set::value_type V_t;
typedef Set::iterator It;
struct info Max;
struct info Min;
void calculateMaxAndMin(Set data)
{
}
void 
