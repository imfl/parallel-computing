// In one minute you can send MAX = 2 orders at most
// If an order to send is blocked, it is rescheduled to the earliest possible time in future

// We maintain 3 queues of orders:
// 1. to_send queue --- orders that are generated
// 2. wait queue --- if an order to send cannot be sent immediately, it is moved to wait queue
// 3. sent queue --- orders sent out

// But the KEY is a queue of time, which records the time of recently sent orders.

#include <cstddef>
#include <iostream>
#include <queue>
#include <random>
 
using std::cout;
using std::endl;
using Time = int;

constexpr Time BEFORE_BEGIN = -1;
constexpr Time BEGIN = 0;
constexpr std::size_t MAX = 2U;

class Order
{
public:
	Order(Time t) : id(++count), time_created(t), time_to_send(t) {}
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
	const Time time_created;
	Time time_to_send;
};

std::size_t Order::count = 0U;

// *** KEY DATA STRUCTURE ***
// We maintain a time queue of recently sent orders, whose length is at most MAX.
// With this queue, we may easily tell, in O(1) time, if we have sent MAX orders in 1 minute or not, because:
// {sent more than MAX orders in the past 1 minute} == {the MAX-th from last was sent within 1 minute from now}
// If the queue is not yet full, we let in orders. This happens at the opening of a trading day.
// If the queue is full, then for one order in (pushed to back), we move one order out (popped from front).
class Order_Time_Queue : public std::queue<Time>
{
public:
	Order_Time_Queue() = default;
	Time get_front_time()
	{
		return empty() ? BEFORE_BEGIN : front();
	}
	void add_to_queue(const Order & od)
	{
		if (size() == MAX)
			pop();
		push(od.get_time_to_send());
	}
};

// decide whether to let go or block an order
bool let_go(Order & od, Order_Time_Queue & recent)
{
	Time t = od.get_time_to_send();
	Time s = recent.get_front_time();
	// if the queue is not yet full (MAX == 2 in this case), or
	// if the front and back orders are not in the same one minute,
	// we let go that order, and push it to a queue of time of orders that are sent recently
	if (recent.size() < MAX || t - s > 0) {
		recent.add_to_queue(od);
		return true;
	}
	// if an order is blocked, we reschedule it to the earliest possible time
	else {
		od.reschedule(s + 1);
		return false;
	}
}

void show_order_info(const Order & od)
{
	cout << "Order " << od.get_id() << " (created at min " << od.get_time_created() <<") to send at min " << od.get_time_to_send() << "\t: ";
}

// process the pseudo stream of orders
void process(std::queue<Order> & to_send, std::queue<Order> & wait, std::queue<Order> & sent, Order_Time_Queue & recent)
{
	cout << "::::::::::::::::::::Decision Process::::::::::::::::::::::" << endl;
	// we process wait queue first, after which we process to_send queue
	// if an order in the to_send queue is blocked, it will be moved to the wait queue
	while (!wait.empty() || !to_send.empty()) {
		if (!wait.empty()) {
			auto od = wait.front();
			show_order_info(od);
			if (let_go(od, recent)) {
				cout << "let go" << endl;
				sent.push(od);
			}
			else {
				// logically impossible for pseudo orders, but possible for real-time orders
				cout << "If you see this line, you've committed a logical error." << endl;
			}
			wait.pop();
		}
		else {
			auto od = to_send.front();
			show_order_info(od);
			if (let_go(od, recent)) {
				cout << "let go" << endl;
				sent.push(od);
			}
			else {
				cout << "BLOCK" << endl;
				wait.push(od);
			}
			to_send.pop();
		}
	}
}

void report(std::queue<Order> & sent)
{
	cout << "::::::::::::::::::::Summary of Results::::::::::::::::::::" << endl;
	while (!sent.empty()) {
		auto od = sent.front();
		cout << "Order " << od.get_id() << " is sent at min " << od.get_time_to_send() << endl;
		sent.pop();
	}
}

void generate_orders_to_send(std::queue<Order> & to_send)
{
	std::default_random_engine e(0);
	std::bernoulli_distribution b(0.5);
	Time t = BEGIN;
	for (int i = 0; i != 100; ++i) {
		if (b(e)) 
			++t;
		to_send.push(Order(t));
	}
}

int main()
{
	std::queue<Order> to_send;
	std::queue<Order> wait;
	std::queue<Order> sent;
	Order_Time_Queue recent;

	generate_orders_to_send(to_send);
	process(to_send, wait, sent, recent);
	report(sent);

	return 0;
}