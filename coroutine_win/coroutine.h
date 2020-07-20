#ifndef _COROUTINE_H_
#define _COROUTINE_H_

#include<memory>
#include<functional>
#include<queue>
#include<unordered_map>
#include<windows.h>

namespace winco {

	/**
	* @brief Э���࣬����һ��Э��ʵ��
	*/
	class coroutine : public std::enable_shared_from_this<coroutine>
	{
	public:
		friend class std::shared_ptr<coroutine>;
	//��������
	public:
		typedef std::function<void(void*)> call_back;
		typedef std::shared_ptr<coroutine> ptr;
		/**
		 * @brief Э��״̬
		 */
		enum class State {
			init,		//��ʼ��
			running,	//������
			suspend,	//����
			term		//����
		};
		
	//��̬�ӿ�
	public:
		//Ĭ�ϵ�Э��ջ��С
		const static size_t default_stack_size = 1024 * 1024;
		/**
		* @brief ����һ��Э�̲����ع���ָ��
		* @param func			Э�̻ص�����
		* @param args			Э�̻ص���������
		 * @param stack_size	Э��ջ��С��0��ΪĬ�ϴ�С
		*/
		static coroutine::ptr create_coroutine(call_back func, void* args, size_t stack_size = 0);
		/**
		 * @brief ��ȡ��ǰ���е�Э��
		 * @return ָ��ǰ����Э�̵Ĺ���ָ��
		 */
		static ptr get_current_coroutine();
		/**
		* @brief ��ȡ��ǰ���е�Э������������Э�̺������û�Э��
		* @return ��ǰ���е�Э����
		*/
		static int get_coroutine_number();
		//��������ָ���deleter
		static void deleter(coroutine* co);

	//����ӿ�
	public:	
		/**
		* @brief ����Э�̣���ʱ�᷵�ص���Э��
		*/
		void yield();

		/**
		 * @brief ����Э�̣�ֻ������Э�̵��ã��˲����������Э�̲����ѵ�ǰЭ��
		 */
		void resume();

		/**
		 * @brief �ر�Э�̣��رպ��Э�̽����ܱ�����\n
		 * ע��:�ر�Э�̲���������ɾ��Э�̶��󣬶��ǻ��鵱ǰ����ɾ��������\n
		 * ��������Э���У����Զ�ɾ��������ɾ�������е�Э�̡�
		 */
		void close();

		/**
		 * @brief ��ȡָ������Ĺ���ָ��
		 */
		ptr get_this();

		/**
		 * @brief ��ȡЭ��״̬
		 * @return Э��״̬
		 */
		State get_state();

		/**
		 * @bried ��ȡЭ��id
		 * @return Э��id
		 */
		int get_id();

		
	private:
		coroutine(call_back func, void* args, size_t stack_size);
		coroutine();
		coroutine(coroutine&) = default;
		~coroutine();
		void destroy_self();

	private:
		//Э��id
		int m_id;
		//Э�̵�����״̬
		State m_state;
		//Э�����еĻص�����
		call_back cb_func;
		//�ص������ĵ��ò���
		void* cb_args;
		//ָ��windows�˳̵�ָ��
		PVOID m_fiber;
		//�Ƿ��Ѿ���destroy����fiber��ϵͳ����ʱ����Ϊtrue
		bool is_destroyed;

	//˽�о�̬��
	private:
		//Э�̳أ�id��Э��ָ���ӳ�䣬idΪ0������Э��
		static std::unordered_map<int, coroutine::ptr> co_pool;
		//��Э��ָ��
		static coroutine::ptr main_coroutine;
		//��ǰ���е�Э��
		static ptr current_coroutine;
		//��ǰЭ����Ŀ
		static int coroutine_num;
		//��ɾ����Э�̶���
		static std::queue<coroutine::ptr> delete_queue;
		//Э����ں�����������ת��Э�̻ص�����
		static void CALLBACK coroutine_main_func(LPVOID lpFiberParameter);
		//�Զ�ɾ��������Э��
		static void auto_delete();

	};

}


#endif


