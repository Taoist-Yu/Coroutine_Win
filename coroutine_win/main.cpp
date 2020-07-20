#include<iostream>
#include<algorithm>
#include "coroutine.h"
using namespace std;
using namespace winco;



void cb_foo(void*)
{
	for (int i = 0; i < 10; i++) {
		cout << "task" << i << ", co_id:" << coroutine::get_current_coroutine()->get_id()<<endl;
		coroutine::get_current_coroutine()->yield();
	}
}

int main()
{
	coroutine::ptr co1 = coroutine::create_coroutine(cb_foo,nullptr);
	coroutine::ptr co2 = coroutine::create_coroutine(cb_foo, nullptr);
	while (co1->get_state() != coroutine::State::term) {
		co1->resume();
		if(co2->get_state() != coroutine::State::term)
			co2->resume();
	}
	getchar();
	return 0;
}