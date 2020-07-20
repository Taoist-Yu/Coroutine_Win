#include "coroutine.h"
#include<cstdlib>
#include<ctime>

#define MAX_COROUTINE_NUM 32768

using winco::coroutine;

#pragma region ��̬����

//Э�̳أ�id��Э��ָ���ӳ�䣬idΪ0������Э��
std::unordered_map<int, coroutine::ptr> coroutine::co_pool;
//��Э��ָ��
coroutine::ptr coroutine::main_coroutine;
//��ǰ���е�Э��
coroutine::ptr coroutine::current_coroutine;
//��ǰЭ����Ŀ
int coroutine::coroutine_num;
//��ɾ����Э�̶���
std::queue<coroutine::ptr> coroutine::delete_queue;

#pragma endregion

#pragma region ��Ա����

//Ĭ�Ϲ��캯����������Э�̻���������Э��
coroutine::coroutine()
{
	//��ʼ�������
	srand(time(NULL));

	//��ʼ��Э������
	this->m_id = 0;
	this->m_state = State::running;
	this->cb_func = nullptr;
	this->cb_args = nullptr;
	this->is_destroyed = false;

	//���̴߳��д���ת��Ϊ��Э��
	this->m_fiber = ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
	if (this->m_fiber == nullptr) {
		throw "Failed to initialize coroutine environment";
	}
	//��ʼ��Э�̳ؽ��ں������غ�ִ��(��Ϊ��ʱ����ָ��δ��ʼ�����޷���Э�̳��������ָ��)
}

//�ù��캯�����ڴ���һ���µ�Э��
coroutine::coroutine(call_back func, void* args, size_t stack_size)
	: cb_func(func), cb_args(args)
{
	//����Ƿ��Ѿ���ʼ��Э�̻���
	if (main_coroutine == nullptr) {
		coroutine::ptr p(new coroutine(),deleter);
		//��ʼ��Э�̳�
		co_pool.insert({ 0,p });
		main_coroutine = p;
		current_coroutine = p;
		coroutine_num = 1;
	}

	//����Э��id
	int rand_init_id = rand() % MAX_COROUTINE_NUM;
	int id = rand_init_id + 1;
	while (id != rand_init_id && co_pool.count(id)) {
		//����hash������̽���㷨������ʹ�õ�Э��id
		id = (id + 1) % MAX_COROUTINE_NUM;
	}
	if (id != rand_init_id) {
		m_id = rand_init_id;
	}
	else {
		throw "failed to create coroutine, the number of coroutines is up to limit";
	}

	//����Э��
	stack_size = stack_size ? stack_size : default_stack_size;
	m_fiber = CreateFiberEx(stack_size,
		stack_size,
		FIBER_FLAG_FLOAT_SWITCH,
		coroutine_main_func,
		&m_id
	);
	this->is_destroyed = false;

	//��ʼ��Э��״̬������Э�̳�
	m_state = State::init;
	//���캯�����غ����Э�̳�(��Ϊ��ʱ����ָ��δ��ʼ�����޷���Э�̳��������ָ��)
}

coroutine::~coroutine()
{
	if (m_id != 0) {
		DeleteFiber(this->m_fiber);
	}
}

//����ǰЭ�̲����˵���Э��
void coroutine::yield()
{
	//�ò�������Э����Ч
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

//����Э��(�˲��������������ִ�е�Э��)
void coroutine::resume()
{
	//�ò�������Э����Ч
	if (m_id == 0) {
		throw "The operation is invalid to the main coroutine";
	}
	if (get_state() == State::init || get_state() == State::suspend) {
		current_coroutine->m_state = State::suspend;
		current_coroutine = shared_from_this();
		current_coroutine->m_state = State::running;
		SwitchToFiber(current_coroutine->m_fiber);
		//������˵�����Э�̣����Զ�ɾ��ʧЧ�˵�Э��
		if (current_coroutine == main_coroutine) {
			coroutine::auto_delete();
		}
	}
	else {
		throw "Coroutine state error!";
	}
}

//����Э��
void coroutine::close()
{
	//�ò�������Э����Ч
	if (m_id == 0) {
		throw "The operation is invalid to the main coroutine";
	}
	//��Э�̴�Э�̳�����ɾ�����У������µ�ǰЭ����Ŀ
	co_pool.erase(m_id);
	delete_queue.push(shared_from_this());
	coroutine_num--;
	//����Э��״̬
	m_state = State::term;
	//���Э���������У����˻���Э��
	if (current_coroutine == shared_from_this()) {
		current_coroutine = main_coroutine;
		main_coroutine->m_state = State::running;
		SwitchToFiber(main_coroutine->m_fiber);
	}
}

//��ȡ������ָ��
coroutine::ptr coroutine::get_this()
{
	return shared_from_this();
}

//��ȡЭ��״̬
coroutine::State coroutine::get_state()
{
	return m_state;
}

//��ȡЭ��id
int coroutine::get_id()
{
	return m_id;
}

//Э����ɱ(��auto_delete()�б�����)
void coroutine::destroy_self()
{
	//�ͷ�Э����Դ
	DeleteFiber(this->m_fiber);
}

#pragma endregion

#pragma region ��̬����

//����Э��
coroutine::ptr coroutine::create_coroutine(call_back func, void* args, size_t stack_size)
{
	coroutine::ptr cp(new coroutine(func, args, stack_size), coroutine::deleter);
	co_pool.insert({ cp->m_id,cp });
	coroutine_num++;
	return cp;
}

//Э����ں���������Э�̶��������룬Ȼ����ת��Э�̻ص�����
void CALLBACK coroutine::coroutine_main_func(LPVOID lpFiberParameter)
{
	//��ȡ��ǰЭ��
	int curID = *(int*)lpFiberParameter;
	coroutine::ptr co = co_pool[curID];
	//����Э�̻ص�����
	co->m_state = State::running;
	co->cb_func(co->cb_args);
	//Э��ִ����ϣ�Э������
	co->close();
	current_coroutine = main_coroutine;
	SwitchToFiber(main_coroutine->m_fiber);
}

//��ȡ��ǰ���е�Э��ָ��
coroutine::ptr coroutine::get_current_coroutine()
{
	return current_coroutine;
}

//��ȡ��ǰЭ����Ŀ
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

