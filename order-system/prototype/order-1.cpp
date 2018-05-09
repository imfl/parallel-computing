// In one minute you can send MAX = 2 orders at most
// If an order to be sent is blocked, it is delayed (rescheduled) to the earliest possible time

#include <cstddef>
#include <iostream>
#include <queue>
#include <random>
#include <vector>
 
using std::cout;
using std::endl;
using Time = int;

constexpr std::size_t MAX = 2U;

class Order
{
public:
	Order(Time t) : id(count++), time_created(t), time_to_send(t) {}
	std::size_t get_id() const
	{
		return id;
	}
	Time get_time_created()	const
	{
		return time_created;
	}
	Time get_time_to_send()	const
	{
		return time_to_send;
	}
	void reschedule(Time t)
	{
		time_to_send = t;
	}
private:
	std::size_t id;
	static std::size_t count;
	Time time_created;
	Time time_to_send;
};

std::size_t Order::count = 1U;

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
		push(od.get_time_to_send());
	}
};

// decide whether to let go or block an order
bool f(Order & od, Order_Queue & q)
{
	Time t = od.get_time_to_send();
	Time s = q.get_head_time();
	// if the queue is not full (MAX == 2 in this case),
	// or if the head and tail are not in the same one minute,
	// we let go that order, and push it on a queue of orders that are sent
	if (q.size() < MAX || t - s > 0) {
		q.add_to_queue(od);
		return true;
	}
	// if an order is blocked, we reschedule it to the earliest possible time
	else {
		od.reschedule(s + 1);
		return false;
	}
}

int main()
{
	// generate a pseudo stream of orders and save them in vector u
	// start at time 0
	Time t = 0;
	std::vector<Order> u;
	std::default_random_engine e(0);
	std::bernoulli_distribution b(0.5);
	for (int i = 0; i != 100; ++i) {
		// this is equivalent to: traders send 2 orders in one minute on average
		if (b(e)) 
			++t;
		u.push_back(Order(t));
	}

	// deal with the pseudo stream of orders and save the decisions in vector v
	cout << "::::::::::::::::::::Decision Process::::::::::::::::::::::" << endl;
	Order_Queue q;
	std::vector<Order> v;
	for (auto it = u.begin(); it != u.end();) {
		cout << "Order " << it->get_id() << " (created at min " << it->get_time_created() <<") to send at min " << it->get_time_to_send() << "\t: ";
		// if an order is let go, we move on to the next order in the stream
		// if an order is blocked, we reschedule it to the earliest possible time
		if (f(*it, q)) {
			cout << "let go" << endl;
			v.push_back(*it);
			++it;
		}
		else {
			cout << "BLOCK" << endl;
		}
	}

	// report summary of results
	cout << "::::::::::::::::::::Summary of Results::::::::::::::::::::" << endl;
	for (const auto & od : v)
		cout << "Order " << od.get_id() << " is sent at min " << od.get_time_to_send() << endl;

	return 0;
}