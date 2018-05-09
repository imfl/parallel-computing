// In one minute you can send MAX = 2 orders at most

#include <iostream>
#include <queue>
#include <random>
#include <vector>
 
using namespace std;
using Time = int;

const int MAX = 2;

std::default_random_engine e(0);
std::bernoulli_distribution b(0.5);

class Order
{
public:
	Order(Time t) : time(t) {}
	Time get_time()	const
	{
		return time;
	}
private:
	Time time;
};

class Order_Queue : public std::queue<Time>
{
public:
	Order_Queue() = default;
	Time get_head_time()
	{
		return size() == 0 ? 0 : front();
	}
	void add_to_queue(const Order & od)
	{
		if (size() == MAX)
			pop();
		push(od.get_time());
	}
};

bool f(const Order & od, Order_Queue & q)
{
	Time t = od.get_time();
	Time s = q.get_head_time();
	if (q.size() < MAX || t - s > 0) {
		q.add_to_queue(od);
		return true;
	}
	else
		return false;
}
int main()
{
	Time t = 0;
	vector<Order> v;
	for (int i = 0; i != 100; ++i) {
		if (b(e)) 
			++t;
		v.push_back(Order(t));
	}
	Order_Queue q;
	for (auto & od : v) {
		cout << "Order at min " << od.get_time() << "\t: ";
		if (f(od, q))
			cout << "let go" << endl;
		else
			cout << "BLOCK" << endl;
	}
}