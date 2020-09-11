#include "thread_pool.h"
#include<cstdio>

void CTask::setData(void* data){
	m_ptrData= data;
}

//静态成员初始化
vector<CTask*> CThreadPool::m_vecTaskList;	//用vector构造任务队列
bool CThreadPool::shutdown=false;		//设置线程退出标志为false
pthread_mutex_t CThreadPool::m_pthreadMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t CThreadPool::m_pthreadCond = PTHREAD_COND_INITIALIZER;

//线程管理类构造函数
CThreadPool::CThreadPool(int threadNum){
	this->m_iThreadNum = threadNum;
	printf("I will create %d threads.\n",threadNum);
	create();	
}

//线程回调函数
void * CThreadPool::ThreadFunc(void* threadData){
	pthread_t tid=pthread_self();	//获取当前线程id
	while(1){
	
	pthread_mutex_lock(&m_pthreadMutex);//取引用对互斥量加锁
	//如果任务队列为空，等待新任务进入任务队列
	while(m_vecTaskList.size()==0&&!shutdown)
		pthread_cond_wait(&m_pthreadCond,&m_pthreadMutex);//对服务线程池进行加锁等待，当前可用任务为0，需要等待wait操作，资源数--,防止多个线程请求
	//关闭线程
	if(shutdown)
	{
		pthread_mutex_unlock(&m_pthreadMutex);	//线程结束任务释放临界区
		printf("[tid: %lu]\texit\n",pthread_self());
		pthread_exit(NULL);
	}

	printf("[tid: %lu]\trun:",tid);
	vector<CTask*>::iterator it=m_vecTaskList.begin();
	//取出一个任务并处理之
	CTask *task=*it;
	if(it!=m_vecTaskList.end())
	{
		task=*it;
		m_vecTaskList.erase(it);
	}
	
	pthread_mutex_unlock(&m_pthreadMutex);		//释放临界区
	
	task->Run();		//执行任务
	printf("[tid: %lu]\tidle\n",tid);		//当前线程空闲
	
		


	}
	
	return (void*) 0;
	
}


//往任务队列中添加任务发出线程同步信号
int CThreadPool::AddTask(CTask *task){
	pthread_mutex_lock(&m_pthreadMutex);	//临界区资源加锁
	m_vecTaskList.push_back(task);
	pthread_mutex_unlock(&m_pthreadMutex);
	pthread_cond_signal(&m_pthreadCond);	//释放一个等待任务的队列
	
	return 0;	

}


//创建线程
int CThreadPool::create(){
	pthread_id=new pthread_t[m_iThreadNum];
	for(int i=0;i<m_iThreadNum;i++)
		pthread_create(&pthread_id[i],NULL,ThreadFunc,NULL);
	return 0;
}


//停止所有线程
int CThreadPool::StopAll(){
	//避免重复调用
	if(shutdown)
		return -1;
	printf("Now I will end all threads!\n\n");

	//唤醒所有等待任务的线程，线程池也要销毁
	shutdown =true;
	pthread_cond_broadcast(&m_pthreadCond);//唤醒所有等待任务的线程
	
	//清除僵尸进程，以阻塞的方式等待thread指定的线程结束
	for(int i=0;i<m_iThreadNum;i++)
		pthread_join(pthread_id[i],NULL);//主线程等待服务线程结束
	delete []pthread_id;
	pthread_id=NULL;

	//销毁互斥量和条件变量
	pthread_mutex_destroy(&m_pthreadMutex);
	pthread_cond_destroy(&m_pthreadCond);

	return 0;	
}

//获取当前队列中的任务数
int CThreadPool::getTaskSize(){
	return m_vecTaskList.size();
}











