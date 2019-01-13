#include <iostream>
#include <thread>
#include <chrono>

#include "cpc.h"

using namespace rot::cpc;

void print_cloudcoins(const std::vector<CloudCoin>& coins);
void print_nresponses(const std::vector<NodeResponse>& resps);
std::mutex mtx;

int main()
{
	if (auto [coins, rr] = read_json("/home/vanya/cloudcoin/payment/old/test.stack"); !rr.code) {
		std::cout << "=== Print cloudcoins ===\n";
		print_cloudcoins(coins);
		std::cout << rr.expl << '\n'; //обязательно перенос
		//return 0;	

		std::cout << "=== Test detect ===\n";
		std::thread t2([&coins] {
			for (auto it = coins.begin(); it != coins.end(); ++it) {
				auto [dstate, drep] = detect(*it);
				if (!drep.code) {
					print_cloudcoins({*it});

					std::vector<NodeResponse> dstate_vec;
					std::copy(begin(dstate), end(dstate), std::back_insert_iterator {dstate_vec});
					print_nresponses(dstate_vec);
				} else
					std::cout << drep.code << '\n' << drep.expl << '\n';
			}
		});

		std::cout << "=== Test echo ===\n";
		std::thread t1([] {
			std::this_thread::sleep_for(std::chrono::milliseconds(5'000));
			if (auto [statuses, echo_rep] = echo({{1, 1}, {1, 2}, {1, 3}}, false); !echo_rep.code) {
				print_nresponses(statuses);
			} else
				std::cout << echo_rep.code << '\n' << echo_rep.expl << '\n';
		});

		//образуется очередь из request(). Дескриптор общий, сохраняет кэш.
		t1.join();
		t2.join();
		print_cloudcoins(coins);
	} else
		std::cout << rr.code << '\n' << rr.expl << '\n';	

	//int* n[3] {new int(1), new int(2), new int(3)};
	//int* p = n + 2;
	//p = new(4);
	//int *p = n + 2;
	
	//std::cout << *p << std::endl;

}

void print_nresponses(const std::vector<NodeResponse>& resps)
{
	std::lock_guard<std::mutex> locker(mtx);

	for (auto it = resps.begin(); it != resps.end(); ++it) {
		std::cout << "START RESP\n";
		if (!it->get_report().code) {
			std::cout 
				<< it->get_server() << '\n'
				<< it->get_status() << '\n'
				<< it->get_message() << '\n'
				<< it->get_time() << '\n';
		} else {
			std::cout << it->get_report().code << '\n' << it->get_report().expl << '\n';
		}
		std::cout << "END RESP\n";
	}
}

void print_cloudcoins(const std::vector<CloudCoin>& coins)
{
	std::lock_guard<std::mutex> locker(mtx);

	for (auto it = coins.begin(); it != coins.end(); ++it) {
		std::cout << "START COIN\n";
		std::cout
			<< it->sn << '\n'
			<< it->nn << '\n'
			<< it->pown << '\n'
			<< it->ed.get_string() << '\n';
		for (
				auto anit = it->ans.begin(); 
				anit != it->ans.end(); 
				++anit)
			std::cout << *anit << '\n';
		for (
				auto aoidit = it->aoid.begin(); 
				aoidit != it->aoid.end(); 
				++aoidit)
			std::cout << *aoidit << '\n';
		std::cout << "END COIN\n";
	}
}
