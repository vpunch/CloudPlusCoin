#include <iostream>

#include "cpc.h"

using namespace rot::cpc;

void print_cloudcoins(std::vector<CloudCoin> coins);
void print_nresponses(std::vector<NodeResponse> reps);

int main()
{
	if (auto [coins, rr] = read_json("/home/vanya/cloudcoin/payment/old/test.stack"); !rr.code) {
		std::cout << "=== Print cloudcoins ===\n";
		print_cloudcoins(coins);
		std::cout << rr.expl << '\n'; //обязательно перенос
		//return 0;

		std::cout << "=== Test echo ===\n";
		if (auto [statuses, echo_rep] = echo(false); !echo_rep.code) {
			print_nresponses(statuses);
		} else
			std::cout << echo_rep.code << '\n' << echo_rep.expl << '\n';

		std::cout << "=== Test detection ===\n";
		for (auto it = coins.begin(); it != coins.end(); ++it) {
			auto [dstate, drep] = detection(*it);
			if (!drep.code) {
				print_cloudcoins({*it});
				print_nresponses(dstate);
			} else
				std::cout << drep.code << '\n' << drep.expl << '\n';
		}

	} else
		std::cout << rr.code << '\n' << rr.expl << '\n';

	//int* n[3] {new int(1), new int(2), new int(3)};
	//int* p = n + 2;
	//p = new(4);
	//int *p = n + 2;
	
	//std::cout << *p << std::endl;

}

void print_nresponses(std::vector<NodeResponse> resps)
{
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

void print_cloudcoins(std::vector<CloudCoin> coins)
{
	for (auto it = coins.begin(); it != coins.end(); ++it) {
		std::cout << "START COIN\n";
		std::cout
			<< it->serial_number << '\n'
			<< it->network_number << '\n'
			<< it->pown << '\n'
			<< it->expiration_date.get_string() << '\n';
		for (
				auto anit = it->authenticity_number.begin(); 
				anit != it->authenticity_number.end(); 
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
