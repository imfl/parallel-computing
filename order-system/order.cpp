// 18/04/25 = Wed

// An trading order system with traffic flow control

// 一种具有流量控制功能的交易订单系统

// [COMPILE AND RUN]

// To compile (on Windows): g++ order.cpp -fopenmp -std=c++14 

// To run (on Windows):

// - default mode: a

// - factory mode: a f

// - file redirection, for example: a f < spec\one

#include "order.hpp"

size_t Order::counter = 0;
clock_t Order::begin = clock();

void System::start(bool factory_mode)
{
    if (factory_mode) {
        await("start");
        Order::reset_time_begin();
    }
    line("real time");
    const int NTR = p->get_num_traders();
    # pragma omp parallel num_threads(NTR + 2)
    {
        int thread_id = omp_get_thread_num();
        if (thread_id == 0)
            print(to_print, process_done);
        else if (thread_id == NTR + 1)
            process(to_send, sent, to_print, traders_done, process_done);
        else
            generate(thread_id, to_send, to_print,
                     orders_in_progress, orders_by_trader, traders_done);
    }
}

void System::report() const
{
    await("show log");
    line("log");

    for (const auto & order : sent)
        cout << "Order # " << order.get_id()
             << "\tcreated at time " << to_second(order.get_time_created())
             << " by trader # " << order.get_creator()
             << " to send at time " << to_second(order.get_time_to_send())
             << " is sent at time " << to_second(order.get_time_sent()) << endl;

    await("show summary and specifications");
    line("summary");
    const auto NTR = p->get_num_traders();
    for (int i = 0; i != NTR; ++i) {
        cout << "Trader # " << (i+1)
             << "\tcreated " << orders_by_trader[i].size()
             << " order" << (orders_by_trader[i].size() > 1 ? "s" : "")
             << ": Order # ";
        int n = 0;
        for (auto id : orders_by_trader[i]) {
            cout << id << " ";
            if (++n > 10) {
                cout << "...";
                break;
            }
        }
        cout << endl;
    }

    line("specifications");
    const auto NOD = p->get_num_orders_to_gen();
    const auto LEN = p->get_monitor_length();
    const auto MAX = p->get_max_orders();
    const auto CYC = p->get_order_cycle();
    cout << "Number of orders\t" << NOD << endl;
    cout << "Number of traders\t" << NTR << endl;
    cout << "Length of interval\t" << to_second(LEN) << endl;
    cout << "Max orders in interval\t" << MAX << endl;
    cout << "Order generation cycle\t" << to_second(CYC) << endl;
}

void System::generate(int               trader_id,
                      deque<Order> &    to_send,
                      deque<string> &   to_print,
                      int &             orders_in_progress,
                      set<size_t> *     orders_by_trader,
                      int &             traders_done)
{
    const auto NOD = p->get_num_orders_to_gen();
    const auto CYC = p->get_order_cycle();

    default_random_engine e(0);
    bernoulli_distribution b(0.5);

    while (Order::get_orders_created() < NOD) {
        bool permit_to_create = false;
        # pragma omp critical (planning)
        {
            // if: (orders created) + (orders in progress) < (orders to generate)
            //    then: permit to create
            if (Order::get_orders_created() + orders_in_progress < NOD) {
                permit_to_create = true;
                orders_in_progress += 1;
            }
        }
        if (permit_to_create) {
            // random generate
            if (b(e)) {
                Order order(trader_id);
                orders_by_trader[trader_id - 1].insert(order.get_id());

                // print
                ostringstream oss;
                oss << "Order # " << order.get_id() << "\tcreated\tat time "
                    << to_second(order.get_time_created()) 
                    << " by trader # " << order.get_creator() << endl;
                # pragma omp critical (printing)
                {
                    to_print.push_back(oss.str());
                }

                // enqueue: schedule to send
                # pragma omp critical (sending)
                {
                    to_send.push_back(std::move(order));
                }
            }
            # pragma omp critical (planning)
            {
                orders_in_progress -= 1;
            }
        }
        Sleep(CYC);
    }

    # pragma omp critical (finishing)
    {
        traders_done += 1;
    }
}

void System::process(deque<Order> &     to_send,
                     deque<Order> &     sent,
                     deque<string> &    to_print,
                     const int &        traders_done,
                     bool &             process_done)
{
    int NTR = p->get_num_traders();
    while (!(traders_done == NTR && to_send.empty())) {
        # pragma omp critical (sending)
        {
            if (!to_send.empty()) {
                Order & order = to_send.front();
                if (let_go(order, sent)) {
                    // enqueue: has been sent
                    sent.push_back(std::move(order));
                    // stamp with time sent
                    sent.back().set_time_sent(Order::get_time_now());
                    // dequeue from schedule
                    to_send.pop_front();

                    // print
                    ostringstream oss;
                    oss << "Order # " << sent.back().get_id()
                        << "\tsent\tat time "
                        << to_second(sent.back().get_time_sent())
                        << endl;
                    # pragma omp critical (printing)
                    {
                        to_print.push_back(oss.str());
                    }
                }
            }
        }
    }
    process_done = true;
}

// let go or block an order
bool System::let_go(Order & order, const deque<Order> & sent)
{
    // have only sent a few orders in total (let go)
    const auto MAX = p->get_max_orders();
    if (sent.size() < MAX)
        return true;

    // not yet time to send (block)
    Time t = order.get_time_to_send();
    if (t > Order::get_time_now())
        return false;

    // ***** KEY *****
    // { sent more than 10 orders in the past 1 second } is equivalent to
    // { the 10-th order from last was sent within 1 second from now }
    Time s = sent[sent.size() - MAX].get_time_sent();

    // has waited for enough time (let go)
    const auto LEN = p->get_monitor_length();
    if (t > s && t - s >= LEN) {
        return true;
    }
    // sent too many orders in too short a time (block and reschedule)
    else {
        order.set_time_to_send(s + LEN);
        return false;
    }
}

void System::print(deque<string> & to_print, const bool & process_done)
{
    while (!(process_done && to_print.empty())) {
        if (!to_print.empty()) {
            # pragma omp critical (printing)
            {
                cout << to_print.front();
                to_print.pop_front();
            }
        }
    }
}

void Spec::configure()
{
    line("factory mode");
    cout << "Enter number of orders to generate"
            " (default = " << NOD << ", max = " << MAX_NOD << "): "; 
    input(NOD, function<bool(const decltype(NOD) &)>
                        ([](const decltype(NOD) & nod)
                        { return 0 < nod && nod <= MAX_NOD; }));

    cout << "Enter number of traders"
            " (default = " << NTR << ",  max = " << MAX_NTR << "): ";
    input(NTR, function<bool(const decltype(NTR) &)>
                        ([](const decltype(NTR) & ntr)
                        { return 0 < ntr && ntr <= MAX_NTR; }));

    cout << "Enter length of monitored interval in milliseconds"
            " (default = " << LEN << " ms): ";
    input(LEN, function<bool(const decltype(LEN) &)>
                        ([](const decltype(LEN) & len)
                        { return 0 < len; }));

    cout << "Enter max number of orders permitted to send in an interval"
            " (default = " << MAX << "): ";
    input(MAX, function<bool(const decltype(MAX) &)>
                        ([](const decltype(MAX) & max)
                        { return 0 < max; }));

    cout << "Enter cycle of order generation in ms"
            " (shorter cycle => faster generation, default = "
         << CYC << " ms): ";
    input(CYC, function<bool(const decltype(CYC) &)>
                        ([](const decltype(CYC) & cyc)
                        { return 0 < cyc; }));
}

template<typename T>
void Spec::input(T & t, function<bool(const T &)> f)
{
    auto s = t;
    string str;
    getline(cin, str);
    istringstream iss(str);
    if (!(cin && iss >> t && f(t))) {
    t = s;
    cin.clear();
    }
    cout << endl;
}

int main(int argc, char * argv[])
{
    bool factory_mode = (argc != 1);
    Spec spec(factory_mode);
    System sys(spec);
    sys.start(factory_mode);
    sys.report();
    return 0;
}