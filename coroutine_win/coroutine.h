#ifndef _COROUTINE_H_
#define _COROUTINE_H_

#include<memory>
#include<functional>
#include<queue>
#include<unordered_map>
#include<windows.h>

namespace winco {

	/**
	* @brief 协程类，代表一个协程实例
	*/
	class coroutine : public std::enable_shared_from_this<coroutine>
	{
	public:
		friend class std::shared_ptr<coroutine>;
	//类型声明
	public:
		typedef std::function<void(void*)> call_back;
		typedef std::shared_ptr<coroutine> ptr;
		/**
		 * @brief 协程状态
		 */
		enum class State {
			init,		//初始化
			running,	//运行中
			suspend,	//挂起
			term		//结束
		};
		
	//静态接口
	public:
		//默认的协程栈大小
		const static size_t default_stack_size = 1024 * 1024;
		/**
		* @brief 创建一个协程并返回共享指针
		* @param func			协程回调函数
		* @param args			协程回调函数参数
		 * @param stack_size	协程栈大小，0则为默认大小
		*/
		static coroutine::ptr create_coroutine(call_back func, void* args, size_t stack_size = 0);
		/**
		 * @brief 获取当前运行的协程
		 * @return 指向当前运行协程的共享指针
		 */
		static ptr get_current_coroutine();
		/**
		* @brief 获取当前运行的协程数，包括主协程和所有用户协程
		* @return 当前运行的协程数
		*/
		static int get_coroutine_number();
		//用于智能指针的deleter
		static void deleter(coroutine* co);

	//对象接口
	public:	
		/**
		* @brief 挂起协程，此时会返回到主协程
		*/
		void yield();

		/**
		 * @brief 唤醒协程，只能由主协程调用，此操作会挂起主协程并唤醒当前协程
		 */
		void resume();

		/**
		 * @brief 关闭协程，关闭后的协程将不能被唤醒\n
		 * 注意:关闭协程并不会立即删除协程对象，而是会检查当前加入删除队列中\n
		 * 当返回主协程中，会自动删除所有在删除队列中的协程。
		 */
		void close();

		/**
		 * @brief 获取指向自身的共享指针
		 */
		ptr get_this();

		/**
		 * @brief 获取协程状态
		 * @return 协程状态
		 */
		State get_state();

		/**
		 * @bried 获取协程id
		 * @return 协程id
		 */
		int get_id();

		
	private:
		coroutine(call_back func, void* args, size_t stack_size);
		coroutine();
		coroutine(coroutine&) = default;
		~coroutine();
		void destroy_self();

	private:
		//协程id
		int m_id;
		//协程的运行状态
		State m_state;
		//协程运行的回调函数
		call_back cb_func;
		//回调函数的调用参数
		void* cb_args;
		//指向windows纤程的指针
		PVOID m_fiber;
		//是否已经被destroy，当fiber被系统回收时设置为true
		bool is_destroyed;

	//私有静态段
	private:
		//协程池，id到协程指针的映射，id为0的是主协程
		static std::unordered_map<int, coroutine::ptr> co_pool;
		//主协程指针
		static coroutine::ptr main_coroutine;
		//当前运行的协程
		static ptr current_coroutine;
		//当前协程数目
		static int coroutine_num;
		//待删除的协程队列
		static std::queue<coroutine::ptr> delete_queue;
		//协程入口函数，负责跳转到协程回调函数
		static void CALLBACK coroutine_main_func(LPVOID lpFiberParameter);
		//自动删除结束的协程
		static void auto_delete();

	};

}


#endif


