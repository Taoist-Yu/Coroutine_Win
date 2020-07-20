#include "coroutine.h"
#include<cstdlib>
#include<ctime>

#define MAX_COROUTINE_NUM 32768

using winco::coroutine;

#pragma region 静态变量

//协程池，id到协程指针的映射，id为0的是主协程
std::unordered_map<int, coroutine::ptr> coroutine::co_pool;
//主协程指针
coroutine::ptr coroutine::main_coroutine;
//当前运行的协程
coroutine::ptr coroutine::current_coroutine;
//当前协程数目
int coroutine::coroutine_num;
//待删除的协程队列
std::queue<coroutine::ptr> coroutine::delete_queue;

#pragma endregion

#pragma region 成员方法

//默认构造函数用来创建协程环境并绑定主协程
coroutine::coroutine()
{
	//初始化随机数
	srand(time(NULL));

	//初始化协程属性
	this->m_id = 0;
	this->m_state = State::running;
	this->cb_func = nullptr;
	this->cb_args = nullptr;
	this->is_destroyed = false;

	//主线程串行代码转化为主协程
	this->m_fiber = ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
	if (this->m_fiber == nullptr) {
		throw "Failed to initialize coroutine environment";
	}
	//初始化协程池将在函数返回后执行(因为此时智能指针未初始化，无法向协程池添加智能指针)
}

//该构造函数用于创建一个新的协程
coroutine::coroutine(call_back func, void* args, size_t stack_size)
	: cb_func(func), cb_args(args)
{
	//检查是否已经初始化协程环境
	if (main_coroutine == nullptr) {
		coroutine::ptr p(new coroutine(),deleter);
		//初始化协程池
		co_pool.insert({ 0,p });
		main_coroutine = p;
		current_coroutine = p;
		coroutine_num = 1;
	}

	//分配协程id
	int rand_init_id = rand() % MAX_COROUTINE_NUM;
	int id = rand_init_id + 1;
	while (id != rand_init_id && co_pool.count(id)) {
		//利用hash的线性探针算法搜索可使用的协程id
		id = (id + 1) % MAX_COROUTINE_NUM;
	}
	if (id != rand_init_id) {
		m_id = rand_init_id;
	}
	else {
		throw "failed to create coroutine, the number of coroutines is up to limit";
	}

	//创建协程
	stack_size = stack_size ? stack_size : default_stack_size;
	m_fiber = CreateFiberEx(stack_size,
		stack_size,
		FIBER_FLAG_FLOAT_SWITCH,
		coroutine_main_func,
		&m_id
	);
	this->is_destroyed = false;

	//初始化协程状态并更新协程池
	m_state = State::init;
	//构造函数返回后更新协程池(因为此时智能指针未初始化，无法向协程池添加智能指针)
}

coroutine::~coroutine()
{
	if (m_id != 0) {
		DeleteFiber(this->m_fiber);
	}
}

//挂起当前协程并回退到主协程
void coroutine::yield()
{
	//该操作对主协程无效
	if (m_id == 0) {
		throw "The operation is invalid to the main coroutine";
	}
	if (get_state() == State::running) {
		m_state = State::suspend;
		current_coroutine = main_coroutine;
		main_coroutine->m_state = State::running;
		SwitchToFiber(main_coroutine->m_fiber);
	}
	else {
		throw "Coroutine state error!";
	}
}

//唤醒协程(此操作将会挂起正在执行的协程)
void coroutine::resume()
{
	//该操作对主协程无效
	if (m_id == 0) {
		throw "The operation is invalid to the main coroutine";
	}
	if (get_state() == State::init || get_state() == State::suspend) {
		current_coroutine->m_state = State::suspend;
		current_coroutine = shared_from_this();
		current_coroutine->m_state = State::running;
		SwitchToFiber(current_coroutine->m_fiber);
		//如果回退到了主协程，会自动删除失效了的协程
		if (current_coroutine == main_coroutine) {
			coroutine::auto_delete();
		}
	}
	else {
		throw "Coroutine state error!";
	}
}

//结束协程
void coroutine::close()
{
	//该操作对主协程无效
	if (m_id == 0) {
		throw "The operation is invalid to the main coroutine";
	}
	//将协程从协程池移至删除队列，并更新当前协程数目
	co_pool.erase(m_id);
	delete_queue.push(shared_from_this());
	coroutine_num--;
	//设置协程状态
	m_state = State::term;
	//如果协程正在运行，则退回主协程
	if (current_coroutine == shared_from_this()) {
		current_coroutine = main_coroutine;
		main_coroutine->m_state = State::running;
		SwitchToFiber(main_coroutine->m_fiber);
	}
}

//获取自身共享指针
coroutine::ptr coroutine::get_this()
{
	return shared_from_this();
}

//获取协程状态
coroutine::State coroutine::get_state()
{
	return m_state;
}

//获取协程id
int coroutine::get_id()
{
	return m_id;
}

//协程自杀(在auto_delete()中被调用)
void coroutine::destroy_self()
{
	//释放协程资源
	DeleteFiber(this->m_fiber);
}

#pragma endregion

#pragma region 静态方法

//创建协程
coroutine::ptr coroutine::create_coroutine(call_back func, void* args, size_t stack_size)
{
	coroutine::ptr cp(new coroutine(func, args, stack_size), coroutine::deleter);
	co_pool.insert({ cp->m_id,cp });
	coroutine_num++;
	return cp;
}

//协程入口函数，所有协程都会从这进入，然后跳转到协程回调函数
void CALLBACK coroutine::coroutine_main_func(LPVOID lpFiberParameter)
{
	//获取当前协程
	int curID = *(int*)lpFiberParameter;
	coroutine::ptr co = co_pool[curID];
	//调用协程回调函数
	co->m_state = State::running;
	co->cb_func(co->cb_args);
	//协程执行完毕，协程死亡
	co->close();
	current_coroutine = main_coroutine;
	SwitchToFiber(main_coroutine->m_fiber);
}

//获取当前运行的协程指针
coroutine::ptr coroutine::get_current_coroutine()
{
	return current_coroutine;
}

//获取当前协程数目
int coroutine::get_coroutine_number()
{
	return coroutine_num;
}

void coroutine::auto_delete()
{
	while (!delete_queue.empty())
	{
		coroutine::ptr cp = delete_queue.front();
		delete_queue.pop();
		cp->destroy_self();
	}
}

void coroutine::deleter(coroutine* co) 
{
	if (!co->is_destroyed && co->m_id != 0) {
		throw "You can't delete a running coroutine";
	}
	if (co->m_id == 0) {
	//	co->destroy_self();
	}
	delete co;
}

#pragma endregion

